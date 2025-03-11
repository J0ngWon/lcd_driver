#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>

/*
 * =======================================================================
 * 주요 함수, 매크로, 구조체 및 멤버 설명
 * =======================================================================
 *
 * [함수 및 매크로]
 *
 * 1. i2c_set_clientdata(struct i2c_client *client, void *data)
 *    - I2C 클라이언트에 드라이버 전용(private) 데이터를 연결합니다.
 *      이후 dev_get_drvdata(dev)를 호출하면 이 데이터를 가져올 수 있습니다.
 *
 * 2. dev_get_drvdata(struct device *dev)
 *    - 이전에 i2c_set_clientdata()로 연결한 드라이버 전용 데이터를 반환합니다.
 *
 * 3. i2c_smbus_write_byte(struct i2c_client *client, u8 data)
 *    - 지정된 I2C 클라이언트(디바이스)에 1바이트 데이터를 전송하는 I2C/SMBus API 함수입니다.
 *      내부적으로 I2C 프로토콜(START, 주소, 데이터, STOP)을 처리합니다.
 *
 * 4. module_i2c_driver(struct i2c_driver driver)
 *    - I2C 드라이버를 커널에 등록하는 매크로로, 모듈 로드시 probe()와 remove()가 자동 호출됩니다.
 *
 * 5. i2c_device_id 및 of_device_id 테이블
 *    - 드라이버가 지원하는 I2C 장치 이름과 Device Tree compatible 문자열을 정의하여,
 *      커널이 적절한 디바이스와 드라이버를 매칭할 수 있도록 합니다.
 *
 * [구조체 및 멤버]
 *
 * 1. struct i2c_client
 *    - I2C 디바이스 정보를 담은 구조체로, 주소(client->addr), 어댑터(client->adapter),
 *      그리고 내부의 device 구조체(client->dev) 등을 포함합니다.
 *
 * 2. struct device
 *    - 디바이스 모델의 기본 구조체로, sysfs 속성, 이름, 드라이버 전용 데이터를 저장합니다.
 *
 * 3. struct lcd_data (드라이버 전용 구조체)
 *    - 이 드라이버에서 LCD와 관련된 상태 정보를 저장하는 구조체입니다.
 *      - struct i2c_client *client: LCD가 연결된 I2C 디바이스 정보를 담습니다.
 *      - char buffer[32]: sysfs를 통해 사용자 입력 문자열(최대 31바이트 + '\0')을 저장합니다.
 *
 * [사용 흐름]
 *
 * - probe() 함수:
 *      1. i2c_client의 dev를 통해 드라이버 전용 데이터를 할당하고,
 *      2. i2c_set_clientdata()로 이 데이터를 연결합니다.
 *      3. LCD 초기화(lcd_init_sequence()) 및 sysfs 파일 생성(device_create_file())을 수행합니다.
 *
 * - sysfs store() 함수:
 *      1. dev_get_drvdata(dev)를 호출하여 드라이버 전용 데이터를 가져옵니다.
 *      2. 입력 문자열을 안전하게 처리한 후, lcd_write_string_wrap()을 통해 LCD에 출력합니다.
 *
 * =======================================================================
 */

/*================= 드라이버 전용 구조체 =================*/
struct lcd_data {
    struct i2c_client *client;
    char buffer[32];    // sysfs 입력 문자열( 31 + '\0')
};

/*=============== PCF8574로 1바이트 전송 ===============*/
static int pcf8574_send(struct i2c_client *client, u8 data)
{
    int ret = i2c_smbus_write_byte(client, data);
   
    return ret;
}

/*=============== 4비트 전송 ===============*/
static void lcd_send_nibble(struct i2c_client *client, u8 nibble, bool rs)
{
    /* 상위 4비트 */
    u8 data = (nibble & 0x0F) << 4; 

    /* RS 설정 */
    // bit1:RS bit2:EN bit3:백라이트
    if (rs)
        data |= 0x01;   // 문자 전송
    data |= 0x08;       // 백라이트 ON

    data |= 0x04;       // EN=1
    if (pcf8574_send(client, data) < 0) return;
    udelay(50);

    data &= ~0x04;      // EN=0
    if (pcf8574_send(client, data) < 0) return;
    udelay(50);
}

/*=============== 명령 전송 (RS=0) ===============*/
static void lcd_write_cmd(struct i2c_client *client, u8 cmd)
{
    lcd_send_nibble(client, cmd >> 4, false);
    lcd_send_nibble(client, cmd & 0x0F, false);
    msleep(2);
}

/*=============== 문자 전송 (RS=1) ===============*/
static void lcd_write_data(struct i2c_client *client, u8 data)
{
    lcd_send_nibble(client, data >> 4, true);
    lcd_send_nibble(client, data & 0x0F, true);
    msleep(1);
}

/*=============== 문자열 전송 ===============*/
static void lcd_write_string_wrap(struct i2c_client *client, const char *str)
{
    int col_count = 0;    

    while (*str) {
        if (*str == '\n') {
            /* 개행 문자('\n')를 만날경우 */
            lcd_write_cmd(client, 0xC0);  
            col_count = 0;
        } else {
            lcd_write_data(client, (u8)*str);
            col_count++;
            /* 16자 이상일경우 줄바꿈 */
            if (col_count >= 16) {
                lcd_write_cmd(client, 0xC0);
                col_count = 0;
            }
        }
        str++;
    }
}

/*=============== LCD 초기화 ===============*/
//HD44780 Datasheet 28p/40p 참조
static void lcd_init_sequence(struct i2c_client *client)
{
    msleep(50);
    lcd_send_nibble(client, 0x03, false);
    msleep(5);
    lcd_send_nibble(client, 0x03, false);
    udelay(200);
    lcd_send_nibble(client, 0x03, false);
    udelay(200);
    lcd_send_nibble(client, 0x02, false);  // 4비트 모드 전환

    lcd_write_cmd(client, 0x28); // 4bit, 2줄, 5x8 폰트트 
    lcd_write_cmd(client, 0x0C); // Display on, Cursor off
    lcd_write_cmd(client, 0x06); // Entry mode: 커서 증가,스크롤 없음음
    lcd_write_cmd(client, 0x01); // Clear display
    msleep(5);
}

/*================ sysfs 콜백 ================*/
/* 읽기: /sys/bus/i2c/devices/1-0027/display 읽어보기기 */
static ssize_t lcd_show(struct device *dev, struct device_attribute *attr,
                        char *buf)
{
    struct lcd_data *data = dev_get_drvdata(dev);
    if (!data)
        return -EINVAL;

    return sprintf(buf, "Current buffer: %s\n", data->buffer);
}
/* 쓰기: echo "문자열" > /sys/bus/i2c/devices/1-0027/display */
static ssize_t lcd_store(struct device *dev, struct device_attribute *attr,
                         const char *buf, size_t count)
{
    struct lcd_data *data = dev_get_drvdata(dev);
    size_t len;
    char *kbuf;

    if (!data)
        return -EINVAL;

    /* 31byte 복사 */
    len = min(count, (size_t)31);
    kbuf = kmalloc(len + 1, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    memcpy(kbuf, buf, len);
    kbuf[len] = '\0';

    /*개행 문자 제거 -n 옵션 제거용 */
    if (len > 0 && kbuf[len-1] == '\n') {
        kbuf[len-1] = '\0';
        len--;
    }

    printk(KERN_INFO "lcd_store: writing \"%s\" to LCD\n", kbuf);

    strcpy(data->buffer, kbuf); //보관

    /* 화면 지우고 새 문자열 출력 + 줄바꿈 */
    lcd_write_cmd(data->client, 0x01);  // Clear display
    msleep(5);
    lcd_write_string_wrap(data->client, kbuf);

    kfree(kbuf);
    return count;  // sysfs 규약: 처리한 바이트 수 반환
}

static DEVICE_ATTR(display, 0664, lcd_show, lcd_store);

/*================ probe() & remove() 함수 ================*/
static int lcd_probe(struct i2c_client *client)
{
    struct lcd_data *data;

    dev_info(&client->dev, "lcd_probe: client= %p\n", client);

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    strcpy(data->buffer, "InitialMsg");

    i2c_set_clientdata(client, data);

    lcd_init_sequence(client);

    /* sysfs 속성 파일 생성 /sys/bus/i2c/devices/1-0027/display*/
    device_create_file(&client->dev, &dev_attr_display);
        

    dev_info(&client->dev, "lcd_probe: successful\n");
    return 0;
}

static void lcd_remove(struct i2c_client *client)
{
    device_remove_file(&client->dev, &dev_attr_display);
    dev_info(&client->dev, "lcd_remove: successful\n");
}

/*================ i2c_device_id & of_device_id ================*/
static const struct i2c_device_id lcd_id[] = {
    { "lcd_i2c", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, lcd_id);

static const struct of_device_id lcd_of_match[] = {
    { .compatible = "mycomp,lcd_i2c" },
    { }
};
MODULE_DEVICE_TABLE(of, lcd_of_match);

/*================ i2c_driver ================*/
static struct i2c_driver lcd_driver = {
    .driver = {
        .name = "lcd_i2c_driver",
        .of_match_table = lcd_of_match,
    },
    .probe    = lcd_probe,
    .remove   = lcd_remove,
    .id_table = lcd_id,
};

module_i2c_driver(lcd_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("J0ngWon");
MODULE_DESCRIPTION("LCD driver"); 
savedcmd_/home/one/i2c/lcd.mod := printf '%s\n'   lcd.o | awk '!x[$$0]++ { print("/home/one/i2c/"$$0) }' > /home/one/i2c/lcd.mod

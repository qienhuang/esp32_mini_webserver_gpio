C:\Users\ect\AppData\Local\Programs\Python\Python36-32\python C:/msys64/home/ect/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port COM4 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 C:/msys64/home/ect/esp/hello_world/build/bootloader/bootloader.bin 0x10000 C:/msys64/home/ect/esp/hello_world/build/hello-world.bin 0x8000 C:/msys64/home/ect/esp/hello_world/build/partitions_singleapp.bin
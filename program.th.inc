.ascii ";     ___                     _   _     \n"
.ascii ";    / _ \  ____   ___   ___ | |_| |__  \n"
.ascii ";   / /_\/ |  _ \ / _ \ / __│| __| '_ \ \n"
.ascii ";  / ╱ ╲ \ | | | | (_) |\__ \| |_| | | |\n"
.ascii ";  \ ╲_/ / |_| |_|\___/ |___/ \__|_| |_|\n"
.ascii ";   \___/ Gnos Forth example code by grz\n"
.ascii "; -------------------------------------->\n"
.ascii "; CONSTANTS\n"
.ascii "; -------------------------------\n"
.ascii ": FB_BASE 1e99a000\n"
.ascii ": FB_SIZE 95ffc ; 1024 * 600 - 4\n"
.ascii ": FB_WIDTH 400 ; 1024\n"
.ascii ": FB_BPP 4\n"
.ascii ": FB_BPP_SHIFT 2\n"
.ascii "\n"
.ascii ": LF a ; newline\n"
.ascii "; -------------------------------\n"
.ascii "; GRAPHICS\n"
.ascii "; -------------------------------\n"
.ascii ": cls >color (FB_SIZE FB_BPP_SHIFT <<) 0 FB_BPP do color swap FB_BASE + ! next\n"
.ascii "\n"
.ascii "; index --\n"
.ascii ": plot FB_WIDTH FB_BASE pix\n"
.ascii "\n"
.ascii "; number --\n"
.ascii ": puti\n"
.ascii "\tdup 0 < dup >is_negative? if negate then dup >value >temp\n"
.ascii "\t0 >digits\n"
.ascii "\tloop\n"
.ascii "\t\tdigits 1 + >digits\n"
.ascii "\t\ttemp a / dup >temp 0= until\n"
.ascii "\n"
.ascii "\t#c allot\n"
.ascii "\tis_negative? if '-' over db! 1 + then\n"
.ascii "\tdigits + 0 over db! >strptr\n"
.ascii "\n"
.ascii "\tloop\n"
.ascii "\t\tstrptr 1 - dup >strptr\n"
.ascii "\t\tvalue a % '0' + over db!\n"
.ascii "\t\tvalue a / dup >value 0= until\n"
.ascii "\n"
.ascii "\tstrptr is_negative? if 1 - then puts\n"
.ascii "; -------------------------------\n"
.ascii "; MEMORY UTILS\n"
.ascii "; -------------------------------\n"
.ascii "; addr. --\n"
.ascii ": mem0 dup >addr dlen 0 swap 1 do 0 swap addr + db! next addr\n"
.ascii "; -------------------------------\n"
.ascii "; ARM UTILS (platform dependent)\n"
.ascii "; -------------------------------\n"
.ascii ": ARM_WFE $e320f002\n"
.ascii "; -------------------------------\n"
.ascii "; U-BOOT STUFF\n"
.ascii "; -------------------------------\n"
.ascii "; u-boot API return value type: include/api_public.h\n"
.ascii ": API_NOERR 0\n"
.ascii ": API_EINVAL 1\n"
.ascii ": API_ENODEV 2\n"
.ascii ": API_ENOMEM 3\n"
.ascii ": API_EBUSY 4\n"
.ascii ": API_EIO 5\n"
.ascii ": API_ESYSC 6\n"
.ascii "\n"
.ascii "; u-boot device type: include/api_public.h\n"
.ascii ": DEV_TYP_NONE 0\n"
.ascii ": DEV_TYP_NET 1\n"
.ascii ": DEV_TYP_STOR 2\n"
.ascii "\n"
.ascii ": DT_STOR_IDE 10\n"
.ascii ": DT_STOR_SCSI 20\n"
.ascii ": DT_STOR_USB 40\n"
.ascii ": DT_STOR_MMC 80\n"
.ascii ": DT_STOR_SATA 100\n"
.ascii "\n"
.ascii "; -- u-boot device_info members access\n"
.ascii ": devinfo_type @\n"
.ascii ": devinfo_cookie 4 + @\n"
.ascii ": devinfo_blockcount 8 + @\n"
.ascii ": devinfo_block_size c + @\n"
.ascii ": devinfo_hwaddr 8 +\n"
.ascii ": devinfo_state 10 + @\n"
.ascii "\n"
.ascii "; -- u-boot device_info addr.\n"
.ascii ": dev_allot #14 allot mem0\n"
.ascii "\n"
.ascii "; u-boot device info addr. --\n"
.ascii ": dev_enum_print\n"
.ascii "\tLF putc\n"
.ascii "\t\"DEVICE INFO:\" puts\n"
.ascii "\tdup >devinfo dev_enum\n"
.ascii "\tLF putc \" dev. type: \" puts\n"
.ascii "\t; print device_info struct content\n"
.ascii "\tdevinfo devinfo_type dup >devtype f & DEV_TYP_STOR = if\n"
.ascii "\t\t\"storage\" puts\n"
.ascii "\t\t\n"
.ascii "\t\tdevtype f0 & {\n"
.ascii "\t\t\tDT_STOR_IDE {\n"
.ascii "\t\t\t\t\" (IDE)\" puts\n"
.ascii "\t\t\t} case\n"
.ascii "\t\t\tDT_STOR_SCSI {\n"
.ascii "\t\t\t\t\" (SCSI)\" puts\n"
.ascii "\t\t\t} case\n"
.ascii "\t\t\tDT_STOR_USB {\n"
.ascii "\t\t\t\t\" (USB)\" puts\n"
.ascii "\t\t\t} case\n"
.ascii "\t\t\tDT_STOR_MMC {\n"
.ascii "\t\t\t\t\" (MMC)\" puts\n"
.ascii "\t\t\t} case\n"
.ascii "\t\t\tDT_STOR_SATA {\n"
.ascii "\t\t\t\t\" (SATA)\" puts\n"
.ascii "\t\t\t} case\n"
.ascii "\t\t\t\" (unknown type: \" puts puti \")\"\n"
.ascii "\t\t} call\n"
.ascii "\n"
.ascii "\t\tLF putc  \" cookie: \" puts\n"
.ascii "\t\tdevinfo devinfo_cookie puti\n"
.ascii "\t\tLF putc  \" block count: \" puts\n"
.ascii "\t\tdevinfo devinfo_blockcount puti\n"
.ascii "\t\tLF putc  \" block size: \" puts\n"
.ascii "\t\tdevinfo devinfo_block_size puti\n"
.ascii "\telse\n"
.ascii "\t\tLF putc  \" cookie: \" puts\n"
.ascii "\t\tdevinfo devinfo_cookie puti\n"
.ascii "\t\tLF putc  \"network (\" puts\n"
.ascii "\t\tdevinfo devinfo_hwaddr puts \")\" puts\n"
.ascii "\tthen\n"
.ascii "\tLF putc  \" state: \" puts\n"
.ascii "\tdevinfo devinfo_state puti\n"
.ascii "\n"
.ascii ": gnosth_sample entrypoint\n"
.ascii "\t; allot U-Boot device_info struct\n"
.ascii "\tdev_allot dup >devinfo\n"
.ascii "\t; enum device and print device infos\n"
.ascii "\tdev_enum_print\n"
.ascii "\t; open device; consider a storage device (no network device)\n"
.ascii "\tdevinfo dev_open {\n"
.ascii "\t\tLF putc \"dev_open: \" puts\n"
.ascii "\t\tAPI_EINVAL {\n"
.ascii "\t\t\t\"API_EINVAL\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\tAPI_EBUSY {\n"
.ascii "\t\t\t\"API_EBUSY\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\tAPI_ENODEV {\n"
.ascii "\t\t\t\"API_ENODEV\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t0 {\n"
.ascii "\t\t\t\"ok\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t\"unknown return value type: \" puts puti\n"
.ascii "\t} call\n"
.ascii "\n"
.ascii "\tLF putc\n"
.ascii "\n"
.ascii "\t; allot a whole block buffer (512b)\n"
.ascii "\t#200 allot >buffer\n"
.ascii "\t; e800 is the unpartitioned space after the first partition (in my case the SD has a single small partition and rest is left as unpartitioned)\n"
.ascii "\t; it is computed as follow (bytes): (partition_start+partition_length)/block_size\n"
.ascii "\t; note that this assume partition start / length is aligned to the start of a block (could be unaligned as well but require a different computation)\n"
.ascii "\te800 1 buffer devinfo dev_read {\n"
.ascii "\t\t\"dev_read: \" puts\n"
.ascii "\t\tAPI_EINVAL {\n"
.ascii "\t\t\t\"API_EINVAL\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\tAPI_ENODEV {\n"
.ascii "\t\t\t\"API_ENODEV\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t0 {\n"
.ascii "\t\t\t\"ok\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t\"unknown return value type \" puts puti\n"
.ascii "\t} call\n"
.ascii "\tLF putc \" blocks read: \" puts puti\n"
.ascii "\tLF putc \" first value read: \" puts buffer @ puti\n"
.ascii "\n"
.ascii "\tLF putc\n"
.ascii "\n"
.ascii "\t; increment first value of the block buffer\n"
.ascii "\tbuffer @ 1 + buffer !\n"
.ascii "\n"
.ascii "\t; write the block (same location as read)\n"
.ascii "\te800 1 buffer devinfo dev_write {\n"
.ascii "\t\t\"dev_write: \" puts\n"
.ascii "\t\tAPI_EINVAL {\n"
.ascii "\t\t\t\"API_EINVAL\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\tAPI_ENODEV {\n"
.ascii "\t\t\t\"API_ENODEV\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\tAPI_EIO {\n"
.ascii "\t\t\t\"API_EIO\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t0 {\n"
.ascii "\t\t\t\"ok\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t\"unknown return value type \" puts puti\n"
.ascii "\t} call\n"
.ascii "\n"
.ascii "\t; close device\n"
.ascii "\tdevinfo dev_close {\n"
.ascii "\t\tLF putc \"dev_close: \" puts\n"
.ascii "\t\tAPI_EINVAL {\n"
.ascii "\t\t\t\"API_EINVAL\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\tAPI_ENODEV {\n"
.ascii "\t\t\t\"API_ENODEV\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t0 {\n"
.ascii "\t\t\t\"ok\" puts\n"
.ascii "\t\t} case\n"
.ascii "\t\t\"unknown return value type \" puts puti\n"
.ascii "\t} call\n"
.ascii "\n"
.ascii "\tloop\n"
.ascii "\t\tgetc\n"
.ascii "\t\tputc\n"
.ascii "\n"
.ascii "\t\tARM_WFE\n"
.ascii "\tagain\n"
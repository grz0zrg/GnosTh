# U-Boot API access from ARM assembly

See *api.inc*

Also see my U-Boot write-up (also include U-Boot patch) [here](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_1_easy_io_with_uboot_for_baremetal_usage/)

Tested with U-Boot *tags/v2024.04* and *tags/v2024.10*, versions after *tags/v2024.04* crash on RPI Zero 1.3 unless *arch/arm/Kconfig* is edited to remove the `imply OF_HAS_PRIOR_STAGE` line for *ARCH_BCM283X*.

There is two implementation of the API which match U-Boot C API samples :

* standard U-Boot API
* standalone

The standalone API is simpler but have fewer features, it is also dodgy as it poke into U-Boot internals so may change heavily in different U-Boot version, i used it early on as it was simpler than the standard API.

*rpi_0_sd* directory have my Das U-Boot build (from *v2024.10* branch) for RPI Zero 1.3 ready to be used / copied to a SD, all caches are enabled and the build is optimized for filesize / boot speed with many stuff disabled, also have my patch to make exception works and extended U-Boot API, it doesn't have a dual framebuffer setup though, also note that it load a *gnos.bin* program name from the SD. (not *program.bin*) The directory also have a RPI configuration file optimized for boot speed. (adapt to your use case !)

*rpi_0_config* directory have the U-Boot build config. file for RPI Zero 1.3 board, it was used to build the U-Boot binary in *rpi_0_sd*.

## WARNING

API address is hardcoded, the address will change with different U-Boot configuration so it must be replaced for your own case.

The API address can be known from U-Boot (if enabled) by reading the *api_address* environment variable.

Note that there is a robust alternative to the hardcoded address by searching for U-Boot API signature. (didn't implement)

The standalone API works a bit differently but also has a hardcoded value, it use a jump table offset computed from U-Boot global_data struct. (see sources)
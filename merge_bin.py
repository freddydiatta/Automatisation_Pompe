Import("env")

def merge_bin_action(source, target, env):
    print("Génération du firmware complet (bootloader + partitions + app) pour Wokwi...")
    env.Execute(
        "\"$PYTHONEXE\" \"$OBJCOPY\" --chip esp32 merge_bin "
        "-o $BUILD_DIR/merged_firmware.bin "
        "--flash_mode dio --flash_freq 40m --flash_size 4MB "
        "0x1000 $BUILD_DIR/bootloader.bin "
        "0x8000 $BUILD_DIR/partitions.bin "
        "0x10000 $BUILD_DIR/firmware.bin"
    )

env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bin_action)

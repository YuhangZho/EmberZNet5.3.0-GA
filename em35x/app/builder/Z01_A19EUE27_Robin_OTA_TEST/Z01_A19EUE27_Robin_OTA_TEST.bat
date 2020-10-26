@echo off
REM Post Build processing for IAR Workbench.
SET PROJECT_DIR=%1%
SET TARGET_BPATH=%2%

echo " "
echo "This converts S37 to Ember Bootload File format if a bootloader has been selected in AppBuilder"
echo " "
@echo on
cmd /c ""%ISA3_UTILS_DIR%\em3xx_convert.exe"  "%TARGET_BPATH%.s37" "%TARGET_BPATH%.ebl" > "%TARGET_BPATH%-em3xx-convert-output.txt"
@echo off
type %TARGET_BPATH%-em3xx-convert-output.txt

echo " "
echo "This creates a ZigBee OTA file if the "OTA Client Policy Plugin" has been enabled.
echo "It uses the parameters defined there.  "
echo " "
@echo on
cmd /c ""%PROJECT_DIR%\..\..\..\tool\image-builder\image-builder-windows.exe" --create %TARGET_BPATH%.ota  --version 256 --manuf-id 0x1160 --image-type 0x0002 --tag-id 0x0000 --tag-file %TARGET_BPATH%.ebl --string "EBL Z01_A19EUE27_Robin_OTA_TEST" > %TARGET_BPATH%-image-builder-output.txt"
@echo off
type %TARGET_BPATH%-image-builder-output.txt


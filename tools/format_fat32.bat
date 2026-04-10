@echo off
echo =============================================
echo  Creating 2x 32GB FAT32 partitions on Disk 2
echo =============================================
echo.

(
echo select disk 2
echo clean
echo create partition primary size=32768
echo format fs=fat32 label=DASHCAM quick
echo assign letter=F
echo create partition primary size=32768
echo format fs=fat32 label=STORAGE quick
echo assign letter=G
) | diskpart

echo.
echo Done!
pause

@echo off

set PATH=%XEDK%\bin\win32;%PATH%;
set INCLUDE=%XEDK%\include\win32;%XEDK%\include\xbox;%XEDK%\include\xbox\sys;%INCLUDE%
set LIB=%XEDK%\lib\win32;%XEDK%\lib\xbox;%LIB%
set _NT_SYMBOL_PATH=SRV*%XEDK%\bin\xbox\symsrv;%_NT_SYMBOL_PATH%

echo.
echo Setting environment for using Microsoft Xbox 360 SDK tools.
echo.

echo Compile mame osd shaders
fxc /Fh osd_vs.h /Tvs_3_0 xenon_osd.hlsl /Evs_main
fxc /Fh osd_ps.h /Tps_3_0 xenon_osd.hlsl /Eps_main
echo Compile 2xXBR shaders
fxc /Fh xbr_2x_vs.h /Tvs_3_0 2xBR-v3.5a.cg /Exbr2x_vs_main
fxc /Fh xbr_2x_ps.h /Tps_3_0 2xBR-v3.5a.cg /Exbr2x_ps_main
echo Compile 5xXBR shaders
fxc /Fh xbr_5x_vs.h /Tvs_3_0 5xBR-v3.5.cg /Exbr5x_vs_main
fxc /Fh xbr_5x_ps.h /Tps_3_0 5xBR-v3.5.cg /Exbr5x_ps_main
echo Compile 5xXBR+CRT shaders
fxc /Fh xbr_5x_crt_vs.h /Tvs_3_0 5xBR-v3.5+CRT.cg /Exbr5xcrt_vs_main
fxc /Fh xbr_5x_crt_ps.h /Tps_3_0 5xBR-v3.5+CRT.cg /Exbr5xcrt_ps_main
cmd

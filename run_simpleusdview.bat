@ECHO OFF
SET CURRENT_FOLDER=%~dp0
SET USDROOT=${USD_ROOT_FOR_BATCH}
SET USDEXTRA=%CURRENT_FOLDER%usd-extra
SET RMANTREE=C:\\Program Files\\Pixar\\RenderManProServer-25.2
SET PXR_PLUGINPATH_NAME=%USDROOT%;%USDROOT%\\plugin\\usd;%USDEXTRA%\\plugin\\usd
SET PYTHONPATH=%USDROOT%\\lib\\python;%USDEXTRA%\\lib\\python;%PYTHONPATH%
SET PATH=%PXR_PLUGINPATH_NAME%;%USDROOT%\\bin;%USDROOT%\\lib;%USDEXTRA%\\bin;%USDEXTRA%\\lib;%RMANTREE%\\bin;%PATH%

REM usdview "C:\Users\paolo\Desktop\openusd\kitchen_rotated_flattened.usd"

${TARGET}.exe

@ECHO OFF
SET USDROOT=${USD_ROOT}
SET RMANTREE=C:\\Program Files\\Pixar\\RenderManProServer-25.2
SET PXR_PLUGINPATH_NAME=%USDROOT%\\plugin\\usd
SET PYTHONPATH=%USDROOT%\\lib\\python;%PYTHONPATH%
SET PATH=%USDROOT%\\bin;%USDROOT%\\lib;%RMANTREE%\\bin;%PATH%
${TARGET}.exe

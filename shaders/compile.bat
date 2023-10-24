REM This is a helper script to compile the shaders

SET GLSLC_PATH=%VULKAN_SDK%\Bin\glslangValidator.exe
REM %~dp0 specifies the full path to a file
SET CURRENT_PATH=%~dp0

REM Add new command for new shader!
REM vertex shaders
%GLSLC_PATH% -V %CURRENT_PATH%shader.vert -o %CURRENT_PATH%vert.spv
%GLSLC_PATH% -V %CURRENT_PATH%quadShader.vert -o %CURRENT_PATH%quadVert.spv
%GLSLC_PATH% -V %CURRENT_PATH%blurShader.vert -o %CURRENT_PATH%blurVert.spv

REM fragment shaders
%GLSLC_PATH% -V %CURRENT_PATH%shader.frag -o %CURRENT_PATH%frag.spv
%GLSLC_PATH% -V %CURRENT_PATH%quadShader.frag -o %CURRENT_PATH%quadFrag.spv
%GLSLC_PATH% -V %CURRENT_PATH%blurShader.frag -o %CURRENT_PATH%blurFrag.spv

REM compute shaders
%GLSLC_PATH% -V %CURRENT_PATH%shader.comp -o %CURRENT_PATH%comp.spv

REM copying the shaders to the output directory
xcopy /I /Y %CURRENT_PATH%*.spv %CURRENT_PATH%..\out\build\x64-Debug\shaders
REM moving the shaders to the compiled directory
xcopy /I /Y %CURRENT_PATH%*.spv %CURRENT_PATH%..\shaders\compiled
del %CURRENT_PATH%*.spv

REM exit
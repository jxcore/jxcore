:: Created by npm, please don't edit manually.
@IF EXIST "%~dp0\jx.cmd" (
  "%~dp0\jx.cmd" "%~dp0\.\node_modules\npm\bin\npm-cli.js" %*
) ELSE (
  jx "%~dp0\.\node_modules\npm\bin\npm-cli.js" %*
)

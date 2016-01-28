if not defined npm_config_node_gyp (
  jx "%~dp0\..\..\node_modules\node-gyp\bin\node-gyp.js" %*
) else (
  jx %npm_config_node_gyp% %*
)

# macOS PKG uninstall notes

Future PKG uninstall should:

- remove installed app files
- unload/remove LaunchDaemon only if Zarya installed it
- preserve `~/Library/Application Support/Zarya/` unless user opts in to full data removal

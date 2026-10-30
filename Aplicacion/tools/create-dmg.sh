#!/bin/zsh
set -euo pipefail

if (( $# != 2 )); then
  print -u2 "Uso: create-dmg.sh <Mya Acustica.app> <carpeta de salida>"
  exit 2
fi
app="$1"
destination="$2"
if [[ ! -d "$app/Contents" ]]; then
  print -u2 "No existe el paquete .app indicado"
  exit 2
fi
plist="$app/Contents/Info.plist"
name=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleDisplayName' "$plist")
version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")
executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")
if [[ "$name" != "Mya Acústica Analyzer" || ! -x "$app/Contents/MacOS/$executable" ]]; then
  print -u2 "Nombre visible o ejecutable incorrecto"
  exit 2
fi
mkdir -p "$destination"
destination=$(cd "$destination" && pwd)
stage=$(mktemp -d)
mount=$(mktemp -d)
trap 'if mount | grep -Fq "$mount"; then hdiutil detach "$mount" -quiet || true; fi; rm -rf "$stage" "$mount"' EXIT
ditto "$app" "$stage/Mya Acústica Analyzer.app"
codesign --force --deep --sign - "$stage/Mya Acústica Analyzer.app"
codesign --verify --deep --strict "$stage/Mya Acústica Analyzer.app"
ln -s /Applications "$stage/Applications"
output="$destination/Mya-Acustica-Analyzer-v$version-macOS.dmg"
hdiutil create -quiet -ov -format UDZO -volname "Mya Acústica Analyzer $version" -srcfolder "$stage" "$output"
hdiutil verify -quiet "$output"
hdiutil attach -quiet -readonly -nobrowse -mountpoint "$mount" "$output"
test -x "$mount/Mya Acústica Analyzer.app/Contents/MacOS/$executable"
test -L "$mount/Applications"
test "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$mount/Mya Acústica Analyzer.app/Contents/Info.plist")" = "$version"
codesign --verify --deep --strict "$mount/Mya Acústica Analyzer.app"
hdiutil detach -quiet "$mount"
shasum -a 256 "$output" > "$output.sha256"
print "$output"

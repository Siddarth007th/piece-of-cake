// Create a real Finder alias with Nori's custom icon, without modifying the signed app.
import AppKit
import Foundation

let args = CommandLine.arguments
if args.count != 5 { fatalError("Usage: install_desktop_alias.swift app alias icon evidence.png") }
let app = URL(fileURLWithPath: args[1])
let destination = URL(fileURLWithPath: args[2])
let alias = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString+".alias")
let icon = URL(fileURLWithPath: args[3])
let manager = FileManager.default
let data = try app.bookmarkData(options: .suitableForBookmarkFile, includingResourceValuesForKeys: nil, relativeTo: nil)
try URL.writeBookmarkData(data, to: alias)
guard let portrait = NSImage(contentsOf: icon), NSWorkspace.shared.setIcon(portrait, forFile: alias.path, options: []) else {
    fatalError("Could not apply Nori's icon to the Desktop alias")
}
let resolved = try URL(resolvingAliasFileAt: alias, options: [.withoutUI, .withoutMounting])
guard resolved.standardizedFileURL == app.standardizedFileURL else { fatalError("Alias does not resolve to installed game") }
if manager.fileExists(atPath: destination.path) {
    let attributes = try manager.attributesOfItem(atPath: destination.path)
    let symbolic = attributes[.type] as? FileAttributeType == .typeSymbolicLink
    let existingAlias = (try? destination.resourceValues(forKeys: [.isAliasFileKey]).isAliasFile) == true
    guard symbolic || existingAlias else { fatalError("Refusing to replace a non-alias Desktop item") }
    try manager.removeItem(at: destination)
}
try manager.copyItem(at: alias, to: destination)
var visibleDestination = destination
var visibility = URLResourceValues()
visibility.isHidden = false
try visibleDestination.setResourceValues(visibility)
try manager.removeItem(at: alias)
let rendered = NSWorkspace.shared.icon(forFile: destination.path)
rendered.size = NSSize(width: 256, height: 256)
let bitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: 256, pixelsHigh: 256, bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
NSGraphicsContext.saveGraphicsState()
NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: bitmap)
rendered.draw(in: NSRect(x: 0, y: 0, width: 256, height: 256))
NSGraphicsContext.restoreGraphicsState()
try bitmap.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: args[4]))
print("Desktop Finder alias resolves to the installed game; custom character icon applied.")

import CoreGraphics
// Prints 1 when the console session's screen is locked, else 0.
let d = CGSessionCopyCurrentDictionary() as? [String: Any]
print((d?["CGSSessionScreenIsLocked"] as? Bool ?? false) ? "1" : "0")

# VFS volumes

KOS now uses a volume namespace rather than one hard-coded root filesystem.
Volumes have a single drive letter and a filesystem backend, so paths are
stable when the backend later changes from RAM/initramfs to a block-backed
FAT32 volume.

## Current volumes

| Volume | Label | Backend | Persistence |
| --- | --- | --- | --- |
| `C:` | `System` | Read-only boot initramfs | Rebuilt into the boot image. |
| `D:` | `Data RAM` | Separate read-only RAM filesystem | Lost on reboot. |

The `D:` volume is an intentional VFS test backend, not a fake disk. It proves
that volume routing and file lookup are independent from the `C:` initramfs.

## Terminal commands

```text
KOS> vol
KOS> ls C:
KOS> ls D:
KOS> cat C:\README.TXT
KOS> cat D:\README.TXT
```

Both `/` and `\` are accepted after a drive letter. VFS rejects traversal
components such as `..` and does not let a path switch volumes after parsing.

## Next storage steps

1. Add a generic block request and buffer-cache interface.
2. Add GPT/MBR partition objects.
3. Mount a read-only FAT32 backend as a drive letter, for example `E:`.
4. Add directories and a carefully tested writable filesystem policy.

No user application or shell must change when `D:` is replaced by a persistent
block-backed volume.

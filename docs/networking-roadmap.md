# Networking roadmap

## Current implementation

KOS has a bounded, kernel-owned UDP socket core for the IPv4 loopback endpoint
`127.0.0.1`. It supports opening, binding, sending, receiving, queueing, and
closing UDP sockets. Boot runs a self-test that transfers a payload between two
distinct loopback sockets.

Terminal diagnostics:

```text
KOS> netinfo
KOS> udptest
```

The QEMU launcher now attaches a deterministic Intel e1000 NIC to QEMU's
user-mode NAT/DHCP/DNS backend. KOS discovers that PCI device and enables its
memory/bus-master bits, but has not yet programmed e1000 receive/transmit rings.
This is not external networking. `netinfo` explicitly reports that no external
interface exists. Therefore there is deliberately no `curl` command yet.

## Required path to `curl`

1. Program e1000 receive/transmit descriptor rings and expose an Ethernet frame
   boundary. A virtio-net backend can follow after this deterministic QEMU path.
2. Add ARP, IPv4 parsing/serialization, checksum validation, routing, and ICMP.
3. Add DHCP client support and a fixed-IP test mode.
4. Move the UDP loopback socket API onto the IPv4 packet path.
5. Add TCP state machine, retransmission timers, receive windows, and ports.
6. Add DNS resolver and URL parser in user space or the future shell runtime.
7. Add an HTTP/1.1 client command, initially `curl http://...` only.
8. Add TLS in user space before claiming HTTPS support.

The next visible network commands after a virtio-net backend are `ifconfig`,
`ping`, `ip route`, `nslookup`, and `curl`. Each requires an observable QEMU
test; none should be exposed as a fake success command.

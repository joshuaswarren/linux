Thunderbolt networking validation on Apple Silicon
====================================================

This plan validates Linux Thunderbolt host-to-host networking on Apple Silicon. It does not authorize a port, mode, or driver change while that port carries the USB-PD VDM reset path.

Prerequisites
-------------

* Use Chris Kearney's linux-aurora tree and a current supported release; do not use AsahiLinux kernel sources.
* Confirm the target SoC's ACIO/NHI node, resources, interrupt mappings, power domains, resets, clocks, and Type-C/USB4 topology from that machine's device tree and IOService/ADT captures. Do not infer T600x/T602x wiring from T8103.
* Confirm CONFIG_USB4, the Apple SoC NHI driver, and CONFIG_USB4_NET are enabled in the built kernel. Confirm the module is present before attempting a load.
* Identify cable and port ownership. Record the exact interface, peer, and reset method for each endpoint.

Build and static validation
---------------------------

1. Build the kernel and DTBs with W=1 using the repository's documented build procedure.
2. Run make dtbs_check for changed device-tree files and resolve new warnings/errors.
3. Run scripts/checkpatch.pl --strict on the patch.
4. Inspect the final kernel config and verify CONFIG_USB4_NET=m (or explicitly intended built-in setting) and all required Apple NHI dependencies.
5. Record source commit, config, build command, logs, and resulting kernel/DTB hashes.

Safe live-link validation
-------------------------

For each peer pair, register the experiment and record the baseline before touching any driver, port, or mode:

1. Capture kernel version/config, /sys/bus/thunderbolt/devices, Type-C/altmode state, relevant dmesg, and interface/route state.
2. Run the pair's documented VDM reset-tool detection/no-op and preserve output. Do not issue a reset.
3. Before changing a reset-cable port, driver, or mode, stop unless detection confirms the reset peer is available. Confirm again immediately after the change. If detection fails, restore the prior safe state and stop; never continue with the machine isolated from its reset path.
4. Load thunderbolt_net only after the owner approves the change and the reset baseline passes. Do not change firmware security, authorize unrelated devices, or alter network services.
5. Confirm XDomain discovery and the network interface on both hosts. Record peer identity, interface names, routes, link state, and negotiated MTU. Use a private /30 only if addresses are not already assigned; document and remove only addresses added by this experiment.
6. Verify peer reachability with source/interface-bound ping in both directions, then run iperf3 in both directions with one and four streams. Repeat on each host's LAN path for comparison. Preserve raw outputs and report RTT, throughput, loss, and MTU.
7. If the link is unstable, stop tests, undo only experiment-owned state, and verify the VDM reset path again. Never reboot or reset as a benchmark step.

Acceptance
----------

A link passes only when XDomain/network interfaces appear on both hosts, source-bound traffic succeeds in both directions, and throughput/RTT are measured on both Thunderbolt and LAN. Reset-peer detection must pass before and after every port/mode/driver change and after cleanup. Device-tree enumeration or a healthy status response without real traffic is not a pass.

For macOS interoperability, confirm Thunderbolt Bridge/interface addresses and scoped routes on macOS, then use the same bidirectional RTT and throughput procedure. Do not change existing macOS network services; any added service must be additive and explicitly recorded.

Record
------

Keep dated preregistration, command/output logs, device-tree and IOService captures, build inputs, hashes, interface/route state, and cleanup results with the lab notebook. Report measured values with the exact host, interface, direction, MTU, command, and duration. Do not report unmeasured speeds or treat a successful build as a live-link pass.

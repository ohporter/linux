#
# Generic framework for controlling remote processors
#

obj-$(CONFIG_REMOTEPROC)		+= remoteproc.o
obj-$(CONFIG_REMOTEPROC_SYSFS)		+= remoteproc_sysfs.o
remoteproc-y				:= remoteproc_core.o
remoteproc-y				+= remoteproc_debugfs.o
remoteproc-y				+= remoteproc_virtio.o
remoteproc-y				+= remoteproc_elf_loader.o
obj-$(CONFIG_OMAP_REMOTEPROC)		+= omap_remoteproc.o
obj-$(CONFIG_B6502_REMOTEPROC)		+= b6502_remoteproc.o

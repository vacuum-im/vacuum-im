#unix:!macx {
#  HEADERS += client/linux/handler/exception_handler.h
#  HEADERS += client/linux/crash_generation/crash_generation_client.h
#  HEADERS += client/linux/handler/minidump_descriptor.h
#  HEADERS += client/linux/minidump_writer/minidump_writer.h
#  HEADERS += client/linux/minidump_writer/line_reader.h
#  HEADERS += client/linux/minidump_writer/linux_dumper.h
#  HEADERS += client/linux/minidump_writer/linux_ptrace_dumper.h
#  HEADERS += client/linux/minidump_writer/directory_reader.h
#  HEADERS += client/linux/log/log.h
#  HEADERS += client/minidump_file_writer-inl.h
#  HEADERS += client/minidump_file_writer.h
#  HEADERS += common/linux/linux_libc_support.h
#  HEADERS += common/linux/eintr_wrapper.h
#  HEADERS += common/linux/ignore_ret.h
#  HEADERS += common/linux/file_id.h
#  HEADERS += common/linux/memory_mapped_file.h
#  HEADERS += common/linux/safe_readlink.h
#  HEADERS += common/linux/guid_creator.h
#  HEADERS += common/linux/elfutils.h
#  HEADERS += common/linux/elfutils-inl.h
#  HEADERS += common/using_std_string.h
#  HEADERS += common/memory.h
#  HEADERS += common/basictypes.h
#  HEADERS += common/memory_range.h
#  HEADERS += common/string_conversion.h
#  HEADERS += common/convert_UTF.h
#  HEADERS += google_breakpad/common/minidump_format.h
#  HEADERS += google_breakpad/common/minidump_size.h
#  HEADERS += google_breakpad/common/breakpad_types.h
#  HEADERS += processor/scoped_ptr.h
#  HEADERS += third_party/lss/linux_syscall_support.h
#  SOURCES += client/linux/crash_generation/crash_generation_client.cc
#  SOURCES += client/linux/handler/exception_handler.cc
#  SOURCES += client/linux/handler/minidump_descriptor.cc
#  SOURCES += client/linux/minidump_writer/minidump_writer.cc
#  SOURCES += client/linux/minidump_writer/linux_dumper.cc
#  SOURCES += client/linux/minidump_writer/linux_ptrace_dumper.cc
#  SOURCES += client/linux/log/log.cc
#  SOURCES += client/minidump_file_writer.cc
#  SOURCES += common/linux/linux_libc_support.cc
#  SOURCES += common/linux/file_id.cc
#  SOURCES += common/linux/memory_mapped_file.cc
#  SOURCES += common/linux/safe_readlink.cc
#  SOURCES += common/linux/guid_creator.cc
#  SOURCES += common/linux/elfutils.cc
#  SOURCES += common/string_conversion.cc
#  SOURCES += common/convert_UTF.c
#  #breakpad app need debug info inside binaries
#  QMAKE_CXXFLAGS+=-g
#}


#mac {
#  HEADERS += client/mac/handler/exception_handler.h
#  HEADERS += client/mac/crash_generation/crash_generation_client.h
#  HEADERS += client/mac/crash_generation/crash_generation_server.h
#  HEADERS += client/mac/crash_generation/client_info.h
#  HEADERS += client/mac/handler/minidump_generator.h
#  HEADERS += client/mac/handler/dynamic_images.h
#  HEADERS += client/mac/handler/breakpad_nlist_64.h
#  HEADERS += client/mac/handler/mach_vm_compat.h
#  HEADERS += client/minidump_file_writer.h
#  HEADERS += client/minidump_file_writer-inl.h
#  HEADERS += common/mac/macho_utilities.h
#  HEADERS += common/mac/byteswap.h
#  HEADERS += common/mac/MachIPC.h
#  HEADERS += common/mac/scoped_task_suspend-inl.h
#  HEADERS += common/mac/file_id.h
#  HEADERS += common/mac/macho_id.h
#  HEADERS += common/mac/macho_walker.h
#  HEADERS += common/mac/macho_utilities.h
#  HEADERS += common/mac/bootstrap_compat.h
#  HEADERS += common/mac/string_utilities.h
#  HEADERS += common/linux/linux_libc_support.h
#  HEADERS += common/string_conversion.h
#  HEADERS += common/md5.h
#  HEADERS += common/memory.h
#  HEADERS += common/using_std_string.h
#  HEADERS += common/convert_UTF.h
#  HEADERS += processor/scoped_ptr.h
#  HEADERS += google_breakpad/common/minidump_exception_mac.h
#  HEADERS += google_breakpad/common/breakpad_types.h
#  HEADERS += google_breakpad/common/minidump_format.h
#  HEADERS += google_breakpad/common/minidump_size.h
#  HEADERS += third_party/lss/linux_syscall_support.h

#  SOURCES += client/mac/handler/exception_handler.cc
#  SOURCES += client/mac/crash_generation/crash_generation_client.cc
#  SOURCES += client/mac/crash_generation/crash_generation_server.cc
#  SOURCES += client/mac/handler/minidump_generator.cc
#  SOURCES += client/mac/handler/dynamic_images.cc
#  SOURCES += client/mac/handler/breakpad_nlist_64.cc
#  SOURCES += client/minidump_file_writer.cc
#  SOURCES += common/mac/macho_id.cc
#  SOURCES += common/mac/macho_walker.cc
#  SOURCES += common/mac/macho_utilities.cc
#  SOURCES += common/mac/string_utilities.cc
#  SOURCES += common/mac/file_id.cc
#  SOURCES += common/mac/MachIPC.mm
#  SOURCES += common/mac/bootstrap_compat.cc
#  SOURCES += common/md5.cc
#  SOURCES += common/string_conversion.cc
#  SOURCES += common/linux/linux_libc_support.cc
#  SOURCES += common/convert_UTF.c
#  LIBS += /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation
#  LIBS += /System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices
#  #breakpad app need debug info inside binaries
#  QMAKE_CXXFLAGS+=-g
#}

win32 {
  HEADERS += common/windows/string_utils-inl.h
  HEADERS += common/windows/guid_string.h
  HEADERS += client/windows/handler/exception_handler.h
  HEADERS += client/windows/common/ipc_protocol.h
  HEADERS += google_breakpad/common/minidump_format.h
  HEADERS += google_breakpad/common/breakpad_types.h
  HEADERS += client/windows/crash_generation/crash_generation_client.h
  HEADERS += processor/scoped_ptr.h

  SOURCES += client/windows/handler/exception_handler.cc
  SOURCES += common/windows/string_utils.cc
  SOURCES += common/windows/guid_string.cc
  SOURCES += client/windows/crash_generation/crash_generation_client.cc
}

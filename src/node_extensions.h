// Copyright & License details are available under JXCORE_LICENSE file

NODE_EXT_LIST_START
NODE_EXT_LIST_ITEM(node_buffer)
#if HAVE_OPENSSL
NODE_EXT_LIST_ITEM(node_crypto)
#endif
NODE_EXT_LIST_ITEM(node_evals)
NODE_EXT_LIST_ITEM(node_fs)
NODE_EXT_LIST_ITEM(node_http_parser)
NODE_EXT_LIST_ITEM(node_os)
NODE_EXT_LIST_ITEM(node_zlib)

// libuv rewrite
NODE_EXT_LIST_ITEM(node_jxtimers_wrap)
NODE_EXT_LIST_ITEM(node_jxutils_wrap)
NODE_EXT_LIST_ITEM(node_memory_wrap)
NODE_EXT_LIST_ITEM(node_thread_wrap)
NODE_EXT_LIST_ITEM(node_module_wrap)
NODE_EXT_LIST_ITEM(node_timer_wrap)
NODE_EXT_LIST_ITEM(node_tcp_wrap)
NODE_EXT_LIST_ITEM(node_udp_wrap)
NODE_EXT_LIST_ITEM(node_pipe_wrap)
NODE_EXT_LIST_ITEM(node_cares_wrap)
NODE_EXT_LIST_ITEM(node_tty_wrap)
NODE_EXT_LIST_ITEM(node_process_wrap)
NODE_EXT_LIST_ITEM(node_fs_event_wrap)
NODE_EXT_LIST_ITEM(node_signal_wrap)

NODE_EXT_LIST_END

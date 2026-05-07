target_sources(PadOS_KDebugConsole PRIVATE
	KConsoleCommand.cpp
	KDebugConsole.cpp
	KSerialMux.cpp
	KSerialMux.h
	KSerialPseudoTerminal.cpp
	KSerialPseudoTerminal.h
)

add_subdirectory(Commands)

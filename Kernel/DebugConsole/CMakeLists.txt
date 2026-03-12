target_sources(PadOS_KDebugConsole PRIVATE
	KConsoleCommand.cpp
	KDebugConsole.cpp
	KSerialPseudoTerminal.cpp
	KSerialPseudoTerminal.h
)

add_subdirectory(Commands)

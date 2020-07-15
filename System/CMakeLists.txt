target_sources(PadOS PRIVATE
  HandleObject.cpp
  SystemMessageIDs.h
)

add_subdirectory(GUI)
add_subdirectory(Math)
add_subdirectory(Ptr)
add_subdirectory(Signals)
add_subdirectory(Threads)
add_subdirectory(Utils)

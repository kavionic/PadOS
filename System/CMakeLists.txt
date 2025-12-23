target_sources(PadOS_System PRIVATE
  HandleObject.cpp
  NewlibRetargetLocking.cpp
  Time.cpp
)

add_subdirectory(GUI)
add_subdirectory(Math)
add_subdirectory(Process)
add_subdirectory(Ptr)
add_subdirectory(Signals)
add_subdirectory(Storage)
add_subdirectory(System)
add_subdirectory(Threads)
add_subdirectory(Utils)

target_sources(PadOS_GUI PRIVATE
	Application.cpp
	Bitmap.cpp
	Color.cpp
	Font.cpp
	KeyboardView.cpp
	LayoutNode.cpp
	Region.cpp
	View.cpp
	ViewFactory.cpp
	ViewFactoryContext.cpp
	ViewScroller.cpp
	Window.cpp
)

add_subdirectory(Dialogs)
add_subdirectory(Widgets)

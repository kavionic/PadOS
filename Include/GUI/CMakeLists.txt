target_sources(PadOS_GUI PRIVATE
	Bitmap.h
	Color.h
	Desktop.h
	Font.h
	GUIDefines.h
	GUIEvent.h
	KeyboardView.h
	LayoutNode.h
	Region.h
	View.h
	ViewBase.h
	ViewFactory.h
	ViewFactoryContext.h
	ViewScroller.h
	Window.h
)

add_subdirectory(Dialogs)
add_subdirectory(Widgets)

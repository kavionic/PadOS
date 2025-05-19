target_sources(PadOS_GUI PRIVATE
	BitmapView.cpp
	Button.cpp
	ButtonBase.cpp
	ButtonGroup.cpp
	CheckBox.cpp
	Control.cpp
	GroupView.cpp
	MVCBaseView.cpp
	MVCGridView.cpp
	MVCListView.cpp
	ProgressBar.cpp
	RadioButton.cpp
	ScrollBar.cpp
	ScrollView.cpp
	Slider.cpp
	TabView.cpp
	TextBox.cpp
	TextEditView.cpp
	TextView.cpp
)

add_subdirectory(DropdownMenu)
add_subdirectory(ListView)
add_subdirectory(Menu)

//Align our chat to the bottom of the window.  If true is passed, view will also be scrolled down
function alignChat(shouldScroll) {
	var windowHeight = window.innerHeight;
	
	if (windowHeight > 0) {
		var contentElement = document.body;
		var diffHeight = (windowHeight - document.documentElement.offsetHeight); 
		if (diffHeight > 0) {
			contentElement.style.position = 'relative';
			contentElement.style.top = diffHeight + 'px';
		} else {
			contentElement.style.position = 'static';
		}
	}
	
	if (shouldScroll) scrollToBottom();
}

function scrollToBottom() {
	window.scrollTo(0,document.body.scrollHeight);
}

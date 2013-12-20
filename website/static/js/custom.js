(function() {
    "use strict"

    $(document).ready(function() {
        $("img.lazy").lazyload({
            effect: "fadeIn"
        });
        var sectionid = "overview";
        $(".navbar-nav li a[href='#" + sectionid + "']").addClass("active");
        autoHighlightNavBar();
        $('.carousel').carousel({
            interval: 2000
        })
    })

    function autoHighlightNavBar() {
        var navItems = $("nav li").children(); // find the a children of the list items
        var hrefs = []; // create the empty hrefs
        for (var i = 0; i < navItems.length; i++) {
            hrefs.push($(navItems[i]).attr("href"));
        }
        $(window).scroll(function() {
            var windowPos = $(window).scrollTop(); // get the offset of the window from the top of page
            var windowHeight = $(window).height(); // get the height of the window
            var docHeight = $(document).height();
            var offset = $("#nav").height(); //navbar offset

            for (var i = 0; i < hrefs.length; i++) {
                var sectionid = hrefs[i];
                var divPos = $(sectionid).offset().top; // get the offset of the div from the top of page
                var divHeight = $(sectionid).height(); // get the height of the div in question
                if (windowPos >= divPos - offset && windowPos < (divPos + divHeight - offset)) {
                    $("a[href='" + sectionid + "']").addClass("nav-active");
                } else {
                    $("a[href='" + sectionid + "']").removeClass("nav-active");
                }
            }

            if (windowPos + windowHeight == docHeight) {
                if (!$("nav li:last-child a").hasClass("nav-active")) {
                    var navActiveCurrent = $(".nav-active").attr("href");
                    $("a[href='" + navActiveCurrent + "']").removeClass("nav-active");
                    $("nav li:last-child a").addClass("nav-active");
                }
            }
        });
    }



}());

/* 
 * LynxBot manual index js
 * note: I don't do javascript
 */

var header1 = document.getElementById("section-1");
var header2 = document.getElementById("section-2");
var header3 = document.getElementById("section-3");
var header4 = document.getElementById("section-4");

var body1 = document.getElementById("man1");
var body2 = document.getElementById("man2");
var body3 = document.getElementById("man3");
var body4 = document.getElementById("man4");

var man1open = false;
var man2open = false;
var man3open = false;
var man4open = false;

var man1 = body1.innerHTML;
body1.innerHTML = "";
var man2 = body2.innerHTML;
body2.innerHTML = "";
var man3 = body3.innerHTML;
body3.innerHTML = "";
var man4 = body4.innerHTML;
body4.innerHTML = "";

function openManSection() {
  if (this === header1) {
    if ((man1open = !man1open))
      body1.innerHTML = man1;
    else
      body1.innerHTML = "";
  } else if (this === header2) {
    if ((man2open = !man2open))
      body2.innerHTML = man2;
    else
      body2.innerHTML = "";
  } else if (this === header3) {
    if ((man3open = !man3open))
      body3.innerHTML = man3;
    else
      body3.innerHTML = "";
  } else {
    if ((man4open = !man4open))
      body4.innerHTML = man4;
    else
      body4.innerHTML = "";
  }
}

header1.addEventListener("click", openManSection, false);
header2.addEventListener("click", openManSection, false);
header3.addEventListener("click", openManSection, false);
header4.addEventListener("click", openManSection, false);

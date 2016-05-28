/* grab authorization code from URL and add to page */
var hash = document.location.hash;
var code = hash.substring(hash.search("=") + 1, hash.search("&"));
document.getElementById("authcode").innerHTML = "<h4>" + code + "</h4>";

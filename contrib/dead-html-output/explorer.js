var dead_starts = [];
var dead_starts_pos = 0;

function insert_dead(deadfile, start, end) {
    dead_starts.push(start);
    for (var i = start; i <= end; i++) {
        $("#line" + i).click(function() { window.location = deadfile; } );
        $("#line" + i).css("cursor", "pointer");
        var color = "#ff6655";
        if (deadfile.match(/undead$/) != null)
            color = "#66dd11";
        $("#line" + i).css("background-color", color);
    }
}

function dead_headers_ready (data) {
    var path = window.location.pathname.split("/");
    var rootFinder = /v2\.6\./;
    while (path.length > 2) {
        if (rootFinder.exec(path.shift())) break;
    }
    var root = data;
    while (path.length > 1) {
        var dir = path.shift();
        if (undefined == root[dir]) return;
        root = root[dir];
    }
    var file = path[0].replace(/\.html$/, "");
    for (var entry in root) {
        if (entry.match("^" + file)) {
            insert_dead(entry, root[entry][0], root[entry][1]);
        }
    }
}

function ScrollTo(pos){
    try{
        scroll(0, pos);
    } catch(e){}
    try{
        window.scrollTo(0, pos);
    }catch(e){}
}

function event_handler(event) {
    if (dead_starts.length == 0) return;
    if (event.which == 110) { // 'n'
        var line = dead_starts[dead_starts_pos];
        dead_starts_pos = (dead_starts_pos + 1) % dead_starts.length;
        ScrollTo($("#line" + line).position().top);
    }
}

function startup() {
    var path = window.location.pathname.replace(/.*v2\.6\.[^\/]*\//, "").replace(".html", "");
    $('<a href="." class="icon up">[parent directory]</a> | ' + path + " | Press \'n\' for next dead block!<hr>").insertBefore('pre');
    $.getJSON('/Research/VAMOS/linux-trees/v2.6.38-rc2/dead-headers', dead_headers_ready);
    $(document).keypress(event_handler);
}

$(document).ready(startup);


var result = confirm('Player '+player+' Win the Game\n\n是否重启游戏？');
if (result) {
    location.href = "http://150.158.146.95:8888/";
} else {
    location.href = "http://150.158.146.95:8888/chess.html";
}
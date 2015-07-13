tempState = null;

Square = function(sElement, bIndex, index){
	//allow squares to know where they are
	this.DOMObject = sElement;
	this.DOMObject.onclick = function(){
		var msg = bIndex.toString() +  index.toString() + "\0";
		console.log(msg);
		console.log("Board: "+ bIndex +"\tSquare: " + index + "\n");
		Game.connection.send(msg);
	};
	this.board = bIndex;
	this.index = index;
};

Board = function(bElement, bIndex){
	var self = this;
	
	this.squares = [];
	Array.prototype.slice.call(bElement.getElementsByClassName("square")).forEach(function(square, index, array){
		self.squares[index] = new Square(square, bIndex, index);
	});
	this.DOMObject= bElement;
    //this.squares = element.getElementsByClassName("square");
};


/*=============================================================================
GAME INITIALIZATION
==============================================================================*/
Game = {};

Game.Initialize = function(){
    Game.version = 10.14;
    Game.beta = 1;
    Game.debug = 1;
    Game.boards = [];
	Game.DOMObject = document.getElementById("game");
	Game.playerSymbols = ['x','o'];
    Array.prototype.slice.call(document.getElementsByClassName("board")).forEach(function(element, index, array){
        Game.boards[index] = new Board(element, index);
    });
	Game.chat = {};
	Game.chat.DOMObject = document.getElementById("shader");
	Game.chat.chatTextInput = document.getElementById("chatText");
	Game.chat.chatHistory = document.getElementById("chatHistory");
	Game.chat.isVisible = true;
	Game.turn = {};
	Game.overlay = {};
	Game.overlay.DOMObject = document.getElementById("overlay");
	Game.connection = new WebSocket("ws://localhost:8080");
	Game.connection.onmessage = function (e) {
		if(e.data.substring(0,5) == "chat:" || e.data.substring(0,5) == "serv:"){
			var span = document.createElement('span');
			if(e.data.substring(0,5) == "chat:"){
				span.classList.add('chatMsg');
			} else {
				span.classList.add('servMsg');
			}
			span.innerHTML = e.data.substring(5);
			Game.chat.chatHistory.appendChild(span);
			//deal with chat
		} else {
			console.log(e.data);
			var state = JSON.parse(e.data);
			for (var i = 0; i < Game.boards.length; i++) {
				Game.boards[i].DOMObject.classList.remove('active');
				for (var j = 0; j < Game.boards[i].squares.length; j++) {
					//clear old value
					Game.boards[i].squares[j].DOMObject.innerHTML = "";
					Game.boards[i].squares[j].DOMObject.classList.remove('lastMove');
					if (state[i][j] > 0) {
						//mark it
						var mark = Game.playerSymbols[state[i][j] - 1]; //0 or 1
						var opMark = Game.playerSymbols[state[i][j] % Game.playerSymbols.length];
						var span = document.createElement('span');
						span.classList.add(mark);
						span.innerHTML = mark;
						Game.boards[i].squares[j].DOMObject.appendChild(span);
					}
				}
			}
			//mark the last square played
			Game.boards[state[9][1]].squares[state[9][2]].DOMObject.classList.add('lastMove');
			var mark = Game.playerSymbols[state[state[9][1]][state[9][2]] - 1];
			var opMark = Game.playerSymbols[state[state[9][1]][state[9][2]] % Game.playerSymbols.length];
			Game.overlay.DOMObject.className = 
				Game.switchClass(Game.overlay.DOMObject, opMark, mark);
			Game.DOMObject.className = 
				Game.switchClass(Game.DOMObject, opMark, mark);
			
			//mark the active board(s)
			if (state[9][0] >= 0) {
				Game.boards[state[9][0]].DOMObject.classList.add('active');
			} else {
				for (var i = 0; i < Game.boards.length; i++) {
					Game.boards[i].DOMObject.classList.add('active');
				}
			}

			//mark the winners
			for(var i = 0;i<Game.boards.length;i++){
				if(state[10][i] & 1){
					Game.boards[i].DOMObject.classList.add('x');
				}
				if(state[10][i] & 2){
					Game.boards[i].DOMObject.classList.add('o');
				}
				if(state[10][i] === 0){
					Game.boards[i].DOMObject.classList.remove('o');
					Game.boards[i].DOMObject.classList.remove('x');
				}
			}
			if(state[10][10] > 0){
				//We have a winner!
				;
			}
		}
	};
	Game.connection.onclose = function (e) {
		var span = document.createElement('span');
		span.classList.add('servMsg');
		span.innerHTML = 'Server closed Connection';
		Game.chat.chatHistory.appendChild(span);
		console.log('Server Closed Connection');
	};
	Game.switchClass = function(element, on, off){
		var classList = element.className;
		if(classList == null){
			return on;
		} else {
			var value = "";
			var classes = classList.split(" ");
			for(var i = 0;i<classes.length;i++){
				if(classes[i] !== off && classes[i] !== on){
					value += classes[i] + " ";
				}
			}
			if(on !== ""){
				value += on;
			}
		}
		return value;
	}
}

window.onload = function(){
    Game.Initialize();


	document.addEventListener('keydown', function(event) {
		if(event.getModifierState("Control") && event.keyCode == 90){
			//ctrl-z
			if(Game.chat.isVisible){
				Game.chat.DOMObject.style.left = '-320px';
				Game.chat.isVisible = false;
				Game.DOMObject.className = Game.switchClass(Game.DOMObject,"","small");
				Game.DOMObject.parentElement.className = Game.switchClass(Game.DOMObject.parentElement, "","small");
			} else {
				Game.chat.DOMObject.style.left = '0px';
				Game.chat.isVisible = true;
				Game.DOMObject.className = Game.switchClass(Game.DOMObject,"small","");
				Game.DOMObject.parentElement.className = Game.switchClass(Game.DOMObject.parentElement, "small","");
			}
		}
	});
}


/*===========================================================================
WEBSOCKET TESTING
===========================================================================*/
/*
function connect(){
	//return new WebSocket('ws://172.25.2.108:8080');
	return new WebSocket('ws://localhost:8080');
}
*/
function chatMsg(e) {
	var msg = Game.chat.chatTextInput.value;
	Game.chat.chatTextInput.value = '';
	Game.connection.send("chat:" + msg + "\0");
	return false;
}


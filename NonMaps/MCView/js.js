var canvas;
var ctx;
var w = 0;
var h = 0;
var timer;
var updateStarted = false;
var touches = [];
var modRanges = [];
var websocket = new WebSocket("ws://localhost:8080/ws/");

class Rect {
	constructor(x,y,w,h) {
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
	}

	contains(x, y) {
		return this.x < x && x < this.x + this.w && this.y < y && y < this.y + this.h;
	}
}

class Point {
	constructor(x, y) {
		this.x = x;
		this.y = y;
	}

	lerp(p1, p2, n) {
		var ax = p1.x;
		var bx = p2.x;
		var ay = p1.y;
		var by = p2.y;
		var x = (1 - n) * ax + n * bx;
		var y = (1 - n) * ay + n * by;
		return new Point(x, y);
	}
}



class ModRange {
	constructor(id, idX, idY) {
		this.id = id;
		this.idX = idX;
		this.idY = idY;
		this.valueX = 0;
		this.valueY = 0;
		this.currentPointerPos = new Point(0, 0);
		this.targetPosition = new Point(0, 0);
	}

	accumilateTouches(touches) {
		let len = touches.length;
		var touchesOfInterest = [];
		let myRect = getModRect(this.id);

		for(var i = 0; i < len; i++) {
			if(myRect.contains(touches[i].pageX, touches[i].pageY)) {
				touchesOfInterest.push(touches[i]);
			}
		}
		return touchesOfInterest;
	}

	sendValues() {
		if(this.id == 0)
			handle2DMc(this);
		else if(this.id == 2 || this.id == 3)
			handle1DMc(this);
	}

	interpolateTowardsTarget(stepSizeInPx=0.1) {
		if(this.currentPointerPos.x != this.targetPosition.x && this.currentPointerPos.y != this.targetPosition.y) {
			this.currentPointerPos = this.currentPointerPos.lerp(this.currentPointerPos, this.targetPosition, stepSizeInPx);
		}
	}

	calculateTargetFromTouches(touches) {
		var filteredToches = this.accumilateTouches(touches);
		var normalizedPointerPos = new Point(0, 0);

		if(filteredToches.length == 0)
			return this.targetPosition;

		for(var i = 0; i < filteredToches.length; i++) {
			normalizedPointerPos.x += filteredToches[i].pageX;
			normalizedPointerPos.y += filteredToches[i].pageY;
		}

		if(filteredToches.length > 0) {
			normalizedPointerPos.x /= filteredToches.length;
			normalizedPointerPos.y /= filteredToches.length;
		}
		return normalizedPointerPos;
	}

	updateValueFromCurrentValues() {
		var rect = getModRect(this.id)
		this.valueX = (((this.currentPointerPos.x - rect.x) / rect.w) * 100).toFixed(1);
		this.valueY = (((this.currentPointerPos.y - rect.y) / rect.h) * 100).toFixed(1);
	}

	update(touches) {
		this.targetPosition = this.calculateTargetFromTouches(touches);
		this.updateValueFromCurrentValues();
	}
}

function updateCanvasSize() {
	var nw = window.innerWidth;
	var nh = window.innerHeight;
	if ((w != nw) || (h != nh)) {
		w = nw;
		h = nh;
		canvas.style.width = w+'px';
		canvas.style.height = h+'px';
		canvas.style.marginTop = '-8px';
		canvas.style.marginLeft = '-8px';
		canvas.style.marginRight = '0px';
		canvas.style.marginBottom = '0px';
		canvas.width = w;
		canvas.height = h;
		document.body.style.margin = "0";
		document.body.style.height = h+'px';
		document.body.style.width = w+'px';
	}
}

function sendToPlayground(id, value) {
	websocket.send("/presets/param-editor/set-mc?id="+id+"&value="+value);
}

function getModRect(id) {
	var ret;
	switch(id) {
		case 0:
			ret = new Rect(0,0,w/2,h/2);
			break;
		case 1:
			ret = new Rect(w/2, 0, w/2, h/2);
			break;
		case 2:
			ret = new Rect(0, h/2, w, h/4);
		break;
		case 3:
			ret = new Rect(0, h/2 + h/4, w, h/4);
		break;
		default:
			ret = new Rect(0, 0, 0, 0);
	}
	return ret;
}

function drawCircleAtPoint(ctx, point, size, color) {
	ctx.beginPath();
	ctx.fillStyle = color;
	ctx.arc(point.x, point.y, size, 0, 2*Math.PI, true);
	ctx.fill();
	ctx.stroke();
}

function drawModRangeValue(ctx, modRange) {
	var rect = getModRect(modRange.id);
	var currentPointerPos = modRange.currentPointerPos;
	if(modRange.id == 2 || modRange.id == 3) {
		currentPointerPos.y = rect.y + rect.h / 2;
	}
	drawCircleAtPoint(ctx, currentPointerPos, 20, "Blue");
	drawCircleAtPoint(ctx, modRange.targetPosition, 10, "Red");
}

function drawModRanges(ctx) {
	var colors = ["#FF0000", "#48FF00", "#ff00b6", "#00ffbb"];

	ctx.beginPath();
	for(var i = 0; i < 4; i++) {
		ctx.fillStyle = colors[i];
		var rect = getModRect(i);
		var modRange = modRanges[i];
		ctx.fillRect(rect.x, rect.y, rect.w, rect.h);
		ctx.fillStyle = "Black";
		if(modRange.idX != null)
			ctx.fillText(modRange.idX + ":" + modRange.valueX, rect.x + rect.w / 2, rect.y + rect.h - 1);
		if(modRange.idY != null)
			ctx.fillText(modRange.idY + ": " + modRange.valueY, rect.x + 10, rect.y + rect.h / 2);
	}
}

function update() {
	if (updateStarted)
		return;

	updateStarted = true;

	updateCanvasSize();
	ctx.clearRect(0, 0, w, h);
	drawModRanges(ctx);
	var i, len = modRanges.length;
	for(i=0; i<len;i++) {
		modRanges[i].update(touches);
		drawModRangeValue(ctx, modRanges[i]);
	}

	updateStarted = false;
}

function handle1DMc(modRange) {
	var value = (modRange.valueX / 100).toFixed(3);
	var mcId;
	if(modRange.id == 2)
		mcId = 245;
	else
		mcId = 246;

	sendToPlayground(mcId, value);
}

function handle2DMc(modRange) {
	var value1 = (modRange.valueX / 100).toFixed(3);
	var value2 = (modRange.valueY / 100).toFixed(3);
	var mc1 = 243;
	var mc2 = 244;

	sendToPlayground(mc1, value1);
	sendToPlayground(mc2, value2);
}

function sendValues() {
	for(var i = 0; i < modRanges.length; i++) {
		var modRange = modRanges[i];
		if(modRange.id == 0)
			handle2DMc(modRange);
		else if(modRange.id == 2 || modRange.id == 3)
			handle1DMc(modRange);
	}
}

var sendTimer;
var interpolationTimer;

function interpolate() {
	for(var i = 0; i < modRanges.length; i++) {
		var modRange = modRanges[i];
		modRange.interpolateTowardsTarget();
	}

	sendValues();
}

function onLoad() {
	canvas = document.getElementById('canvas');
	ctx = canvas.getContext('2d');
	timer = setInterval(update, 1);
	interpolationTimer = setInterval(interpolate, 10);

	modRanges = [new ModRange(0, 'A', 'B'), new ModRange(1, 'Kein', 'Kein'), new ModRange(2, 'C', null), new ModRange(3, 'D', null)];

	canvas.addEventListener('touchstart', function(event) {
  		event.preventDefault();
  		touches = event.touches;
	});

	canvas.addEventListener('touchmove', function(event) {
  		event.preventDefault();
  		touches = event.touches;
	});

	canvas.addEventListener('touchend', function(event) {
		touches = event.touches;
	})
};

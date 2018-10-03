tmpl.regexp = /([\s'\\])(?!(?:[^<]|\[(?!%))*%\>)|(?:\<%(=|#)([\s\S]+?)%\>)|(\<%)|(%\>)/g;

const S_CON = 1;
const S_NEW = 2;
const S_ERR = 4;

const AUTH_OPEN = 0;
const AUTH_PERSONAL = 1;
const AUTH_ENTERPRISE = 2;


const AUTH_NAME = [
	"OPEN",
	"WEP",
	"WPAPSK",
	"WPA2PSK",
	"WPAPSK/WPA2PSK",
	"MAX"
	];



function callJSON(method, url, data, success_cb, error_cb) {
	var xhr = new XMLHttpRequest();
	xhr.open(method, url);
	xhr.setRequestHeader('Content-Type', 'application/json');
	xhr.onload = function () {
		if (xhr.status == 200) {
			if (success_cb)
				success_cb(xhr.responseText);
		} else if (xhr.status != 200) {
			if (error_cb)
				error_cb(xhr.status, xhr.responseText);
		}
	};
	xhr.send(data);
}

function loader(b) {
	document.getElementById('loader_box').style.display = b ? 'block' : 'none';
}

function setContent(name, data) {
	document.getElementById('content').innerHTML = tmpl(name, data);
}

function setContentLabel(text){
	document.getElementById('content_label').innerHTML = text;
}

function loadData(method, url, data, success_cb, error_cb){
	loader(1);
	if(typeof(data)==='object')data=JSON.stringify(data);
	callJSON(method, url, data, function(r){
		loader(0);
		var data = JSON.parse(r);
		success_cb(data);
	}, function (c, r) {
		loader(0);
		error_cb(c,r);
	});
	
}
	
function loadTemplate(method, url, data, tmpl_id, success_cb){
	loadData(method, url, data, function (data) {
		setContent(tmpl_id, data);
		if (success_cb)
			success_cb(data);
	}, function (c, r) {
		alert(r);
	});
}


function reloadContent(){
	var hash = window.location.hash || '#home';
	
	var hash_s = hash.match(/^#([^\?]*)(\?(.*))?$/);
	
	var name = hash_s[1];
	var search = hash_s[3];
	var data = search?JSON.parse('{"' + search.replace(/&/g, '","').replace(/=/g,'":"') + '"}',
				function(key, value) { return key===""?value:decodeURIComponent(value) }):{};
	
	name = name.toLowerCase().replace(/\b[a-z]/g, function(letter) {
		return letter.toUpperCase();
	});
	
	var fun = window["go"+name] || goHome;
	
	fun(data);
}

function redirect(name, data){
	var hash = '#' + name.toLowerCase();
	if(data){
		hash += '?';
		
		var str = [];
		for(var p in data){
			if(data.hasOwnProperty(p)){
			  str.push(encodeURIComponent(p) + "=" + encodeURIComponent(data[p]));
			}
		}
		hash +=  str.join("&");
	}
	
	window.location.hash = hash;
}

window.addEventListener("hashchange", reloadContent);


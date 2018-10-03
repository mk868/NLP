


function goHome() {
	setContentLabel('Home');
	loadTemplate('POST', 'go/summary.json', '', "tmpl-home");
}


function loadScanResult(){
	loadData('POST', 'go/scan.json', null, function (data) {
		if(data.ready){
			setContent("tmpl-scan", data);
		} else {
			loader(1);
			setTimeout(loadScanResult, 1000);
		}
	}, function (c, r) {
		alert(r);
	});
}

function goScan() {
	setContentLabel('Scan results:');
	loadScanResult();
}

function goList() {
	setContentLabel('Saved stations:');
	loadTemplate('POST', 'go/list.json', '', "tmpl-list");
}

function goAdd(data){
	setContentLabel('Add');
	
	setContent("tmpl-edit", {ssid: data.ssid});
	
	document.getElementsByName('auth_type')[data.auth].checked = true;
	
	gui_edit_update();
}

function goEdit(data){
	setContentLabel('Edit');
	loadTemplate('POST', 'go/station-info.json', {ssid: data.ssid}, "tmpl-edit",
	function(d){
		if(d.login){
			document.getElementsByName('auth_type')[AUTH_ENTERPRISE].checked = true;
		} else if(d.password){
			document.getElementsByName('auth_type')[AUTH_PERSONAL].checked = true;
		} else {
			document.getElementsByName('auth_type')[AUTH_OPEN].checked = true;
		}
		
		gui_edit_update();
	});
}

function goDelete(data){
	setContentLabel('Delete');
	loadTemplate('POST', 'go/station-info.json', {ssid: data.ssid}, "tmpl-delete");
}

function goInfo(data){
	setContentLabel('Info');
	loadTemplate('POST', 'go/station-info.json'+(data.netinfo?'?netinfo=1':''), {ssid: data.ssid}, "tmpl-info");
}

function letConnect(ssid){
	loadData('POST', 'go/let-connect.json', {ssid: ssid},
	function(d){
		alert('ok');
		//letRefresh();
	},
	function(c, r){
		alert(r);
	});
}


function letDelete(){
	var ssid = document.getElementById('h_ssid').value;
	loadData('POST', 'go/let-delete.json', {ssid: ssid},
	function(d){
		alert('removed');
		letBack();
	},
	function(c, r){
		alert(r);
	});
}

function letSave(){
	var o = {};
	o.ssid = document.getElementById('h_ssid').value;
	
	if(document.getElementsByName('auth_type')[AUTH_PERSONAL].checked){
		o.password = document.getElementById('auth_type_personal_password').value;
	} else if(document.getElementsByName('auth_type')[AUTH_ENTERPRISE].checked){
		o.login = document.getElementById('auth_type_enterprise_login').value;
		o.password = document.getElementById('auth_type_enterprise_password').value;
	}
	
	
	loadData('POST', 'go/let-save.json', o,
	function(d){
		if(d.status == 'OK'){
			alert('saved');
			redirect('info', { ssid: o.ssid });
		} else if(d.status=='ERROR'){
			alert('error');
		} else {
			alert(d.status+'???');
		}
	},
	function(c, r){
		alert(r);
	});
}

function letBack(){
	history.back();
}

function letRefresh(){
	reloadContent();
}



function gui_edit_update(){
	var radios = document.getElementsByName('auth_type');
	
	for (var i = 0, length = radios.length; i < length; i++) {
		document.getElementById(radios[i].name+'_'+radios[i].value).style.display = radios[i].checked?'block':'none'
    }
}

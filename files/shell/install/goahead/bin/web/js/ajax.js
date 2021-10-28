/**
 * Created by wzq on 2020/10/20.
 */

/*
	gloabal functions
*/
function mouseoverd(obj) {

	if (obj.className == "bg_white") {
        obj.style.background = "#F4F4F4"
    }
}

function mouseoutd(obj) {
	
	if (obj.className == "bg_white") {
        obj.style.background = "#ffffff"
    }
}

function isEmpty(objID, strError) {
    var value = document.getElementById(objID).value;
    if ((value == '') || (value == 'undefined')) {
        $("#" + objID).addClass('highlight-error');
        alert(strError);
		document.getElementById(objID).focus();
        return true;
    } else {
        $("#" + objID).removeClass('highlight-error');
        return false;
    }
}

function notEmpty(objID, strError) 
{
    var value = document.getElementById(objID).value;
    if ((value == '') || (value == 'undefined')) {
        $("#" + objID).addClass('highlight-error');
        alert(strError);
		document.getElementById(objID).focus();
        return false;
    } else {
        $("#" + objID).removeClass('highlight-error');
        return true;
    }
}

function json_string_to_obj(string){
	var obj
	if (typeof(JSON) == 'undefined') {
		obj = eval("("+string+")");
	} else {
		obj = JSON.parse(string);
	}	
	return obj;
}

//退出
function go_exit() 
{
    var strmsg="确定要退出吗?";  
    if (!confirm(strmsg)) {
         window.event.returnValue = false;
	} else {	 
		if (navigator.userAgent.indexOf("MSIE") > 0) {
			if (navigator.userAgent.indexOf("MSIE 6.0") > 0) {
				window.opener = null;
			} else {
				window.parent.location.replace("about:blank");
			}
		} else if (navigator.userAgent.indexOf("Chrome") > 0) { //for Chrome
			window.parent.location.replace("about:blank"); //for frames
		} else {
			window.opener = null;
			window.parent.location.replace("about:blank");
		}
		window.parent.parent.location.replace("about:blank");
	}
}


function preExit() 
{
	$.ajax({
        type: "POST",
        url: "/ajax/mylogout",
		timeout:5000,
        data: "NoUse",
        success: function (result) 
		{
		    return 0;
        },
        error: function (result, status) 
		{
            if (status == 'error') {
               // //alert(status);
            }
            return -1;
        }
    });
}

//重启
function go_reboot()
{
	var strmsg="确定要重启设备吗?";  
    if (!confirm(strmsg)) {
         return false;
	} else {
		$.ajax({
            type: "POST",
            url: "/ajax/go_reboot",
			timeout:5000,
            data: "NoUse",
            success: function (result) {
                var data = result.split("|");
                if (parseInt(data[1]) == 1) {
                    alert("设备已重启！");
                } else {
                    alert("重启失败！请断电重启！");
                }
            },
            error: function (result, status) {
                if (status == 'error') {
                }
                return -1;
            }
        });
	}
}

/*
	---------------------------for importExport.html---------------------------------------
*/
function go_export()
{
    var _form = $("<form>");
    _form.attr("style", "display:none");
    _form.attr("target", "");
    _form.attr("method", "post");
    _form.attr("action", "/action/go_export"); 
	
    var _input = $("<input>"); 
    _input.attr("type", "hidden");
    _input.attr("name", "exportSec");
    _input.attr("id", "exportSec");
    var sec = (new Date()).getMilliseconds();
    _input.attr("value", sec);
    //console.log(sec);
    _form.append(_input);

    var _input2 = $("<input>"); 
    _input2.attr("type", "hidden");
    _input2.attr("name", "exportType");
    _input2.attr("id", "exportType");
    var type = $("#filename").val();   //导出类型
    _input2.attr("value", type);
    //console.log(type);
    _form.append(_input2);
    
    $("body").append(_form);
    _form.submit();
	_form.remove();
}


function go_export_route()
{
    var _form = $("<form>");
    _form.attr("style", "display:none");
    _form.attr("target", "");
    _form.attr("method", "post");
    _form.attr("action", "/action/go_export_route"); 
	
    var _input = $("<input>"); 
    _input.attr("type", "hidden");
    _input.attr("name", "exportData");
    _input.attr("value", (new Date()).getMilliseconds());
    $("body").append(_form);
    _form.append(_input);

    _form.submit();
	_form.remove();
}

function go_export_web()
{
    var _form = $("<form>");
    _form.attr("style", "display:none");
    _form.attr("target", "");
    _form.attr("method", "post");
    _form.attr("action", "/action/go_export_web"); 
	
    var _input = $("<input>"); 
    _input.attr("type", "hidden");
    _input.attr("name", "exportData");
    _input.attr("value", (new Date()).getMilliseconds());
    $("body").append(_form);
    _form.append(_input);

    _form.submit();
    console.log("ok...");
	_form.remove();
}

function go_export_platform()
{
    var _form = $("<form>");
    _form.attr("style", "display:none");
    _form.attr("target", "");
    _form.attr("method", "post");
    _form.attr("action", "/action/go_export_platform"); 
	
    var _input = $("<input>"); 
    _input.attr("type", "hidden");
    _input.attr("name", "exportData");
    _input.attr("value", (new Date()).getMilliseconds());
    $("body").append(_form);
    _form.append(_input);

    _form.submit();
	_form.remove();
}

/*
	---------------------------for version.html---------------------------------------
*/

function go_sysinfo()
{
    $.ajax({
        type: "GET",
        url: "/ajax/go_sysinfo",
        timeout:5000,
        success: function(result){
            var obj = json_string_to_obj(result)
            if (obj && obj.hasOwnProperty("data")) {
                $("#soft_version").html(obj.data.versionInfo.appversion);
                $("#kernel_version").html(obj.data.versionInfo.kernelversion);
            }
        },
        error: function (result, status) {
            if (status == 'error') {
            }
            return -1;
        }
    });	
}

function init_statistics(data)
{
	if (data.hasOwnProperty("uplink")) {
		$("#readBytes").html(data.uplink.readBytes+"(Bytes)");
		$("#routeBytes").html(data.uplink.routeBytes+"(Bytes)");
		$("#sendBytes").html(data.uplink.sendBytes+"(Bytes)");
		$("#sendErrBytes").html(data.uplink.sendErrBytes+"(Bytes)");
		$("#forwardPackages").html(data.uplink.forwardPackages+"(个)");
		$("#routeErrPackages").html(data.uplink.routeErrPackages+"(个)");
	} else {
		$("#readBytes").html('?');
		$("#routeBytes").html('?');
		$("#sendBytes").html('?');
		$("#sendErrBytes").html('?');
		$("#forwardPackages").html('?');
		$("#routeErrPackages").html('?');
	}

	if (data.hasOwnProperty("downlink")) {
		$("#cmdPackages").html(data.downink.cmdPackages+"(个)");
		$("#dataPackages").html(data.downink.dataPackages+"(个)");
		$("#errPackages").html(data.downink.errPackages+"(个)");
	} else {
		$("#cmdPackages").html('?');
		$("#dataPackages").html('?');
		$("#errPackages").html('?');
	}

	if (data.hasOwnProperty("softversion")) {
		$("#soft_version").html(data.softversion.appversion);
		$("#kernel_version").html(data.softversion.kernelversion);
	} else {
		$("#soft_version").html('?');
		$("#kernel_version").html('?');
	}

	if (data.hasOwnProperty("temperature")) {
		$("#board_temp").html(data.temperature.boardTemperature+"(℃)");
	} else {
		$("#board_temp").html('?');
	}
	
	if (data.hasOwnProperty("device")) {
		switch(parseInt(data.device.status)) {
			case 0:
				$("#card_status").html("启动中");
				break;
			case 1:
				$("#card_status").html("成功");
				break;
			case 2:
				$("#card_status").html("加载中");
				break;
			case 3:
				$("#card_status").html("加载成功");
				break;
			case -1:
				$("#card_status").html("未检测到子卡");
				break;
			case -2:
				$("#card_status").html("bit文件加载失败");
				break;
			case -3:
				$("#card_status").html("link失败");
				break;
			default:
				$("#card_status").html("未知错误");
				break;
		}
		
	} else {
		$("#card_status").html('?');
	}
}
function go_statistics()
{
	$.ajax({
        type: "GET",
        url: "/ajax/go_statistics",
        timeout:5000,
        success: function(result){
            var obj = json_string_to_obj(result)
            if (obj && obj.hasOwnProperty("data")) {
                init_statistics(obj.data)
            }
        },
        error: function (result, status) {
            if (status == 'error') {
            }
            return -1;
        }
    });	
}

/*
	---------------------------for userset.html---------------------------------------
*/

function isEqual(str1, str2)
{
    var str1Val = $('#' + str1).val();
    var str2Val = $('#' + str2).val();
    if (str1Val == str2Val) {
        $('#' + str2).removeClass('highlight-error');
        return true;
    } else {
		$('#' + str2).addClass('highlight-error');
		alert('新密码不匹配。');
		return false;
	}
}

//密码修改保存提交验证
function go_passwd_set() 
{
    var message = {
        "OldPassword": "请输入初始密码",
        "NewPassword": "请输入新密码",
        "ConfirmPassword": "请重新输入新密码",
        "Incorrect": "初始密码不正确，请重新输入",
        "Success": "密码已修改",
		"Fail": "密码修改失败"
    };
    var Data={};
    if (notEmpty("OldPassword", message['OldPassword']) && 
        notEmpty("NewPassword", message['NewPassword']) && 
        notEmpty("ConfirmPassword", message['ConfirmPassword']) &&
        isEqual("NewPassword", "ConfirmPassword")) {
        
		Data["OldPassword"] = $("#OldPassword").val();
		Data["NewPassword"] = $("#NewPassword").val();

		$.ajax({
			type: "POST",
			url: "/ajax/go_passwd_set",
			timeout:5000,
			data: Data,
			success: function (result) {
				var data = result.split("|");
                var res = parseInt(data[1]);
				if (res == 0) {
				   alert(message["Success"]);
                   window.parent.parent.location.reload();
				} else if(res == 1){
					alert(message["Incorrect"]);
				} else {
                    alert(message["Fail"]);
                }
			},
			error: function (result, status) {
				if (status == 'error') {
				}
				return -1;
			}
		});
    }
}

/*
	---------------------------for network1g.html network10g.html---------------------------------------
*/

/*
function go_network_set(type)
{	
	var message = {
        "ip_empty": "IP地址不能为空。请重新输入IP地址。",
        "mask_empty": "子网掩码不能为空。请重新输入子网掩码。",
		"port_empty": "端口号不能为空。请重新输入端口号。",
        "ip_invalid": "IP地址不合法。请重新输入IP地址。", 
        "mask_invalid": "子网掩码不合法。请重新输入子网掩码。",
		"port_invalid": "端口号不合法。请重新输入端口号。", 
        "gateway_invalid": "默认网关不合法。请重新输入默认网关。",
        "success": "设置成功,应用程序将重启！",
		"fail":"设置失败！",
		"confirm":"确定要修改网络设置，确认后将重新启动网络！"
    };

	var Data = {};
	var index;
	
	Data["type"] = parseInt(type);
	if (Data["type"] == 1) {
		index = AppConfig.NUM_OF_NET1G;
	} else if (Data["type"] == 10) {
		index = AppConfig.NUM_OF_NET10G;
	} else {
		console.log("go_network_Set para err!");
		return;
	}
	 
    for (var i=0; i < index; i++) {
		
		var IPAddressObj = $("#IPAddress_" + i);
        var NetmaskObj = $("#Netmask_" + i);
        var GatewayObj = $("#Gateway_" + i);
		var PortObj = $("#Port_" + i);
		
		var IPAddress = IPAddressObj.val();	
        var Netmask = NetmaskObj.val();
        var Gateway = GatewayObj.val();
		var Port = PortObj.val();
		
		if (Validator.isEmpty(IPAddress)) {
			IPAddressObj.focus();
			alert(message['ip_empty']);
			return;
		} else {
			if (!Validator.isIP(IPAddress)) {
				IPAddressObj.addClass('highlight-error');
				alert(message['ip_invalid']);
				return;
			} else {
				IPAddressObj.removeClass('highlight-error');
			}
		}
		
		if (Validator.isEmpty(Netmask)) {
			NetmaskObj.focus();
			alert(message['mask_empty']);
			return;
		} else {
			if (!Validator.isIP(Netmask)) {
				NetmaskObj.addClass('highlight-error');
				alert(message['mask_invalid']);
				return;
			} else {
				NetmaskObj.removeClass('highlight-error');
			}
		}
		
		if (!Validator.isEmpty(Gateway)){ 
			if(!Validator.isIP(Gateway)) {
				GatewayObj.addClass('highlight-error');
				alert(message['gateway_invalid']);
				return;
			}
		}
		GatewayObj.removeClass('highlight-error');
	
		if (Validator.isEmpty(Port)) {
			PortObj.focus();
			alert(message['port_empty']);
			return;
		} else {
			if (!Validator.isPort(Port)) {
				PortObj.addClass('highlight-error');
				alert(message['port_invalid']);
				return;
			} else {
				PortObj.removeClass('highlight-error');
			}
		}
		
		Data["IPAddress_" + i] = IPAddress;
        Data["Netmask_" + i] = Netmask;
        Data["Gateway_" + i] = Gateway;
		Data["Port_" + i] = Port;
    }
	console.log(Data);
	if(!confirm(message["confirm"]))
		return;

    $.ajax({
        type: "POST",
        url: "/ajax/go_network_set", 
		timeout:5000,
        data: Data,
        success: function (result) {
			
            var data = result.split("|");
			
            if (parseInt(data[1]) == 1) {  
                alert(message["success"]);
				//window.parent.parent.location.replace(data[2]);
               
            } else {
                 alert(message["fail"]);
            }
        },
        error: function (result, status) {
            if (status == 'error') {
            }
            return -1;
        }
    });
}
*/


function clear_table(id)
{
	var tb = document.getElementById(id);
	if (tb) {
		while (tb.rows.length > 1)  //保留表头
			tb.deleteRow(1);	

		var cells = tb.rows.item(0).cells.length;
		var tr = tb.insertRow(-1);
		var td = tr.insertCell(-1);
		td.colSpan = cells;
		td.innerHTML = '<em style="text-align:cneter">No data</em>';
	}
	
}


/*
	---------------------------网络设置---------------------------------------
*/
function netList_save(data)
{
	var message = {
        "ip_empty": "IP地址不能为空。请重新输入IP地址。",
        "mask_empty": "子网掩码不能为空。请重新输入子网掩码。",
		"port_empty": "端口号不能为空。请重新输入端口号。",
        "ip_invalid": "IP地址不合法。请重新输入IP地址。", 
        "mask_invalid": "子网掩码不合法。请重新输入子网掩码。",
		"port_invalid": "端口号不合法。请重新输入端口号。", 
        "gateway_invalid": "默认网关不合法。请重新输入默认网关。",
        "success": "设置成功,应用程序将重启！",
		"fail":"设置失败！",
		"confirm":"确定要修改网络设置，确认后将重新启动网络！"
    };
		
	var trObj = data.parentNode.parentNode;
	var Data = {};
	for (var i=0; i<trObj.cells.length;i++) {
		switch(i) {
			case 0:
				Data["ifname"] = trObj.cells[i].innerHTML;
				break;
			case 3:
				//console.log(trObj.cells[i].children[0].id);
				//Data["ipaddr"] = trObj.cells[i].children[0].value;
				var IPAddressObj = $("#" + trObj.cells[i].children[0].id);
				var IPAddress = IPAddressObj.val();	
				if (Validator.isEmpty(IPAddress)) {
					IPAddressObj.focus();
					alert(message['ip_empty']);
					return;
				} else {
					if (!Validator.isIP(IPAddress)) {
						IPAddressObj.addClass('highlight-error');
						alert(message['ip_invalid']);
						return;
					} else {
						IPAddressObj.removeClass('highlight-error');
					}
				}
				Data["ipaddr"] = IPAddress;
				break;
			case 4:
				//Data["netmask"] = trObj.cells[i].children[0].value;
				var NetmaskObj = $("#" + trObj.cells[i].children[0].id);
				var Netmask = NetmaskObj.val();
				if (Validator.isEmpty(Netmask)) {
					NetmaskObj.focus();
					alert(message['mask_empty']);
					return;
				} else {
					if (!Validator.isIP(Netmask)) {
						NetmaskObj.addClass('highlight-error');
						alert(message['mask_invalid']);
						return;
					} else {
						NetmaskObj.removeClass('highlight-error');
					}
				}
				Data["netmask"] = Netmask;
				break;
			case 5:
				//Data["gateway"] = trObj.cells[i].children[0].value;
				var GatewayObj = $("#" + trObj.cells[i].children[0].id);
				var Gateway = GatewayObj.val();
				if (!Validator.isEmpty(Gateway)){ 
					if(!Validator.isIP(Gateway)) {
						GatewayObj.addClass('highlight-error');
						alert(message['gateway_invalid']);
						return;
					}
				}
				GatewayObj.removeClass('highlight-error');
				Data["gateway"] = Gateway;
				break;
			case 6:
				//Data["port"] = trObj.cells[i].children[0].value;
				var PortObj = $("#" + trObj.cells[i].children[0].id);
				var Port = PortObj.val();
				if (Validator.isEmpty(Port)) {
					PortObj.focus();
					alert(message['port_empty']);
					return;
				} else {
					if (!Validator.isPort(Port)) {
						PortObj.addClass('highlight-error');
						alert(message['port_invalid']);
						return;
					} else {
						PortObj.removeClass('highlight-error');
					}
				}
				Data["port"] = Port;
				break;
			default:
				break;
		}
	}
	console.log(Data);	

	$.ajax({
        type: "POST",
        url: "/ajax/go_netList_save", 
		timeout:5000,
        data: Data,
        success: function (result) {
			
       		console.log(result);
        },
        error: function (result, status) {
            if (status == 'error') {
            }
            return -1;
        }
    });
}

function callback_netList(dataArray)
{
	var tb = document.getElementById("netTab");
	if (tb && dataArray) {
		while (tb.rows.length > 1)  //保留表头
			tb.deleteRow(1);

		for (var i=0; i < dataArray.length; i++) {
			var tr = tb.insertRow(-1);
			tr.insertCell(-1).innerHTML = dataArray[i].ifname;
			if (dataArray[i].link == "up")
				tr.insertCell(-1).innerHTML = dataArray[i].link + "<image src='img/ethernet.png'/>";
			else
				tr.insertCell(-1).innerHTML = dataArray[i].link + "<image src='img/ethernet_disabled.png'/>";
			tr.insertCell(-1).innerHTML = dataArray[i].speed;
			
			tr.insertCell(-1).innerHTML = "<input type='text' id='ipaddr_"+i+"' value='"+dataArray[i].ipaddr+"'/>";
			tr.insertCell(-1).innerHTML = "<input type='text' id='netmask_"+i+"' value='"+dataArray[i].netmask+"'/>";
			tr.insertCell(-1).innerHTML = "<input type='text' id='gateway_"+i+"'value='"+dataArray[i].gateway+"'/>";
			tr.insertCell(-1).innerHTML = "<input type='text' id='port_"+i+"'value='"+dataArray[i].port+"'/>";
			tr.insertCell(-1).innerHTML = "<button class='btn btn-danger radius' onClick='netList_save(this)'>保存</button>";
		}

		var cells = tb.rows.item(0).cells.length;
		if (tb.rows.length == 1) {
			var tr = tb.insertRow(-1);
			var td = tr.insertCell(-1);
			td.colSpan = cells;
			td.innerHTML = '<em style="text-align:cneter">No data</em>';
		}
	}
}


function go_get_netList() 
{
	$.ajax({
		type: "GET",
		url: "/ajax/go_get_netList",
		timeout:5000,
		success: function (result) {
			console.log(result);
			var obj = json_string_to_obj(result)
            if (obj && obj.hasOwnProperty("data")) {
				callback_netList(obj.data);
            } else {
				clear_table("netTab");
			}
		},
		error: function (result, status) {
			if (status == 'error') {
				clear_table("netTab");
			}
			return -1;
		}
	});
}


/*
	---------------------------槽位设置---------------------------------------
*/

function slotList_save(data)
{
	var message = {
        "success": "保存成功",
		"fail": "保存失败"
    };
		
	var trObj = data.parentNode.parentNode;
	var Data = {};
	for (var i=0; i<trObj.cells.length;i++) {
		switch(i) {
			case 0:
				Data["slotId"] = trObj.cells[i].innerHTML;
				break;
			case 7:
				var enable = 0;
				//console.log(trObj.cells[i].children[0]);
				if (trObj.cells[i].children[0].checked) {
					enable = 1;
				}
				Data["enable"] = enable;
				break;
			default:
				break;
		}
	}
	console.log(Data);	

	$.ajax({
        type: "POST",
        url: "/ajax/go_slotList_save", 
		timeout:5000,
        data: Data,
        success: function (result) {
			
       		console.log(result);
        },
        error: function (result, status) {
            if (status == 'error') {
            }
            return -1;
        }
    });
}

function callback_slotList(dataArray)
{
	var tb = document.getElementById("slotTab");
	if (tb && dataArray) {
		while (tb.rows.length > 1)  //保留表头
			tb.deleteRow(1);

		for (var i=0; i < dataArray.length; i++) {
			var tr = tb.insertRow(-1);
			tr.insertCell(-1).innerHTML = dataArray[i].id;
			tr.insertCell(-1).innerHTML = dataArray[i].status;
			switch (dataArray[i].type) {
				case 1:
					tr.insertCell(-1).innerHTML = "威风";
					break;
				case 2:
					tr.insertCell(-1).innerHTML = "非威风";
					break;
				default:
					tr.insertCell(-1).innerHTML = "未知";
					break
			}
			tr.insertCell(-1).innerHTML = dataArray[i].loadStatus;
			tr.insertCell(-1).innerHTML = dataArray[i].softVersion;
			tr.insertCell(-1).innerHTML = dataArray[i].uplinkBytes+"(Bytes)";
			tr.insertCell(-1).innerHTML = dataArray[i].linkStatus;
			tr.insertCell(-1).innerHTML = "<input type='checkbox' id='slot_switch_"+i+"'/>";
			if (parseInt(dataArray[i].linkSwitch) > 0) {
				document.getElementById("slot_switch_"+i).checked = true;
			} else {
				document.getElementById("slot_switch_"+i).checked = false;
			}
			tr.insertCell(-1).innerHTML = "<button class='btn btn-danger radius' onClick='slotList_save(this)'>保存</button>";
		}
		var cells = tb.rows.item(0).cells.length;
		if (tb.rows.length == 1) {
			var tr = tb.insertRow(-1);
			var td = tr.insertCell(-1);
			td.colSpan = cells;
			td.innerHTML = '<em style="text-align:cneter">No data</em>';
		}
	}
}

function go_get_slotList() 
{
	$.ajax({
		type: "GET",
		url: "/ajax/go_get_slotList",
		timeout:5000,
		success: function (result) {
			var obj = json_string_to_obj(result)
			console.log(result);
            if (obj && obj.hasOwnProperty("data")) {
				callback_slotList(obj.data);
            } else {
				clear_table("slotTab");
			}
		},
		error: function (result, status) {
			if (status == 'error') {
				clear_table("slotTab");
			}
			return -1;
		}
	});
}


/*
	---------------------------客户端信息---------------------------------------
*/

function callback_clientList(dataArray)
{
	var tb = document.getElementById("clientTab");
	if (tb && dataArray) {
		while (tb.rows.length > 1)  //保留表头
			tb.deleteRow(1);

		for (var i=0; i < dataArray.length; i++) {
			var tr = tb.insertRow(-1);
			tr.insertCell(-1).innerHTML = dataArray[i].ipaddr;
			tr.insertCell(-1).innerHTML = dataArray[i].port;
			tr.insertCell(-1).innerHTML = dataArray[i].sendOk+"(Bytes)";
			tr.insertCell(-1).innerHTML = dataArray[i].sendErr+"(Bytes)";
		}
		var cells = tb.rows.item(0).cells.length;
		if (tb.rows.length == 1) {
			var tr = tb.insertRow(-1);
			var td = tr.insertCell(-1);
			td.colSpan = cells;
			td.innerHTML = '<em style="text-align:cneter">No data</em>';
		}
	}
}

function go_get_client() 
{
	$.ajax({
		type: "GET",
		url: "/ajax/go_get_client",
		timeout:5000,
		success: function (result) {
			var obj = json_string_to_obj(result)
			console.log(result);
            if (obj && obj.hasOwnProperty("data")) {
				callback_clientList(obj.data);
            } else {
				clear_table("clientTab");
			}
		},
		error: function (result, status) {
			if (status == 'error') {
				clear_table("clientTab");
			}
			return -1;
		}
	});
}


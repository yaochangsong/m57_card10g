/**
 * Created by caolian on 2017/5/25.
 */

var Validator = {};

/**
 * 校验IP格式
 * @param ip
 * @returns {boolean}
 */
Validator.isIP = function (ip) {
    var re =  /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/
    return re.test(ip);
};

/**
 * 是否是有效的IP
 * @param ip
 * @returns {boolean}
 */
Validator.isIPAddress = function (ip) {
    var type = parseIPType(ip);
    if (Object.isNull(type)) {
        return false;
    }
    switch (type) {
        case "A":
            return Validator.isValidIPOfA(ip);
        case "B":
            return Validator.isValidIPOfB(ip);
        case "C":
            return Validator.isValidIPOfC(ip);
        case "D":
            return Validator.isValidIPOfD(ip);
        case "E":
            return Validator.isValidIPOfE(ip);
    }
    return false;
}


Validator.isValidIPOfA = function (ip) {
    if (!Validator.isIP(ip)) {
        return false;
    }
    return !(/^127\..*$/).test(ip) && !(/^\d+\.0\.0\.0$/).test(ip) && !(/^\d+\.255\.255\.255$/).test(ip);
}

Validator.isValidIPOfB = function (ip) {
    if (!Validator.isIP(ip)) {
        return false;
    }
    return !(/^\d+\.\d+\.0\.0$/).test(ip) && !(/^\d+\.\d+\.255\.255$/).test(ip);
}

Validator.isValidIPOfC = function (ip) {
    if (!Validator.isIP(ip)) {
        return false;
    }
    return !(/^\d+\.\d+\.\d+\.0$/).test(ip) && !(/^\d+\.\d+\.\d+\.255$/).test(ip);
}

Validator.isValidIPOfD = function (ip) {
    return false;
}

Validator.isValidIPOfE = function (ip) {
    return false;
}


/**
 * 是否是有效的掩码地址
 * @param ip
 * @returns {boolean}
 */
Validator.isMaskAddress = function (ip) {
    var r1 = Validator.isIP(ip);
    if (!r1)    return false;
    var bin = IP2Bin(ip);
    if (bin === null) {
        return false;
    }
    return (/^1+0*$/).test(bin);
}

/**
 * 是否是有效的网关地址
 * @param ip
 * @returns {boolean}
 */
Validator.isGatewayAddress = function (ip) {
    return Validator.isIPAddress(ip);
}

/**
 * 是否是有效的DNS地址
 * @param ip
 * @returns {boolean}
 */
Validator.isDNSAddress = function (ip) {
    return Validator.isIPAddress(ip);
}

Validator.isAlias = function (alias) {
    var legalStr = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
    var reg = "^[" + legalStr + "]{1,16}$";  /*匹配1-16个字符的字串*/
    return (new RegExp(reg)).test(alias);
};


Validator.isPort = function (n) {
    if (Validator.isInteger(n)) {
        return ((n>=1) && (n<=65535));
    }
    return false;
};

Validator.isInteger = function (n) {
    if (!Validator.isNumber(n)) {
        return false;
    }
    return n%1 == 0;
};


Validator.isNumber = function (n) {
    if (n === null) return false;
    return !isNaN(n);
};

Validator.isNegative = function (n) {
    if (!Validator.isNumber(n)) {
        return false;
    }
    return n > 0;
};


Validator.isMinus = function (n) {
    if (!Validator.isNumber(n)) {
        return false;
    }
    return n < 0;
};


Validator.isID = function (ID) {
    if (Validator.isInteger(ID)) {
        return ((parseInt(ID) >= 1) && (parseInt(ID) <= 247));
    }
    return false;
};


Validator.isFilterID = function (ID) {
    return Validator.isID(ID);
};


Validator.isSlaveID = function (ID) {
    return Validator.isID(ID);
};


Validator.isValidIP = function (mask, ip) {
    var broadDec = IP2Dec(calcBroadCast(mask, ip));  //广播地址
    var networkDec = IP2Dec(calcNetwork(mask, ip));  //网络地址
    var ipDec = IP2Dec(ip);
    return (ipDec > networkDec) && (ipDec < broadDec);
};



//返回网络地址字符串
function calcNetwork(mask, ip) {
    if (Validator.isIP(mask) && Validator.isIP(ip)) {
        var mask_arr = mask.split(".");
        var ip_arr = ip.split(".");
        var network_arr = [];
        for (var i=0;i<mask_arr.length;i++) {
            var n = (mask_arr[i]-0) & (ip_arr[i]-0);  
            network_arr.push(n); //在数组末尾插入元素
        }
        return network_arr.join(".");//用分隔符将数组成员分开，返回一个字符串
    }
    return null;
}


//返回广播地址字符串
function calcBroadCast(mask, ip) {
    if (Validator.isIP(mask) && Validator.isIP(ip)) {
       // var network = calcNetwork(mask, ip);
        var network_arr = network.split(".");
        var mask_arr = mask.split(".");
        var cast_arr = [];
        for (var i=0;i<mask_arr.length;i++) {
            var n = (network_arr[i]-0) ^ (~(mask_arr[i]-0)) & 255;   //&
            cast_arr.push(n);   
        }
        return cast_arr.join(".");  
    }
    return null;
}


/**
 * IP转为10进制
 * @constructor
 */
function IP2Dec(ip) {
    if (!Validator.isIP(ip)) {
        return null;
    }
    var ip_arr = ip.split(".");
    var dec = (ip_arr[0]-0)*16777216 + (ip_arr[1]-0)*65536 + (ip_arr[2]-0)*256 + (ip_arr[3]-0);
    return dec;
}


function parseIPType(ip) {
    if (!Validator.isIP(ip)) {
        return null;
    }
    var ip_arr = ip.split('.');
    if (Math.elt(1, ip_arr[0]) && Math.elt(ip_arr[0], 127)) {
        return "A";
    }
    if (Math.elt(128, ip_arr[0]) && Math.elt(ip_arr[0], 191)) {
        return "B";
    }
    if (Math.elt(192, ip_arr[0]) && Math.elt(ip_arr[0], 223)) {
        return "C";
    }
    if (Math.elt(224, ip_arr[0]) && Math.elt(ip_arr[0], 239)) {
        return "D";
    }
    if (Math.elt(240, ip_arr[0]) && Math.elt(ip_arr[0], 255)) {
        return "E";
    }
    return null;
}


/**
 * 十进制转为IP
 * @param dec
 * @returns {*}
 */
function dec2IP(dec) {
    if (Validator.isNegative(dec) && Validator.isInteger(dec)) {
        dec = dec - 0;
        var net_arr = [];
        net_arr.push(dec >>> 24);
        net_arr.push((dec << 8) >>> 24);
        net_arr.push((dec << 16) >>> 24);
        net_arr.push((dec << 24) >>> 24);
        return net_arr.join(".");
    }
    return null;
}


/**
 * IP转为二进制
 * @param ip
 * @returns {string}
 * @constructor
 */
function IP2Bin(ip) {
    if (!Validator.isIP(ip)) {
        return null;
    }
    var arr = ip.split(".");
    return (dec2Bin(arr[0])) + (dec2Bin(arr[1])) + (dec2Bin(arr[2])) + (dec2Bin(arr[3]));
}


/**
 * 将十进制转为二进制
 * @param decimal
 * @returns {string}
 */
function dec2Bin(decimal) {
    var bit8 = 0,
        bit7 = 0,
        bit6 = 0,
        bit5 = 0,
        bit4 = 0,
        bit3 = 0,
        bit2 = 0,
        bit1 = 0;

    if (decimal & 128) {
        bit8 = 1
    }
    if (decimal & 64) {
        bit7 = 1
    }
    if (decimal & 32) {
        bit6 = 1
    }
    if (decimal & 16) {
        bit5 = 1
    }
    if (decimal & 8) {
        bit4 = 1
    }
    if (decimal & 4) {
        bit3 = 1
    }
    if (decimal & 2) {
        bit2 = 1
    }
    if (decimal & 1) {
        bit1 = 1
    }
    return ("" + bit8 + bit7 + bit6 + bit5 + bit4 + bit3 + bit2 + bit1);
}

/**
 * 将二进制转为10进制
 * @param n
 * @returns {number}
 */
function bin2Dec(n) {
    var decimal = 0;
    while (binary.length < 8) {
        binary = "0" + binary;
    }
    if (binary.substring(7, 8) == "1") {
        decimal++
    }
    if (binary.substring(6, 7) == "1") {
        decimal = decimal + 2
    }
    if (binary.substring(5, 6) == "1") {
        decimal = decimal + 4
    }
    if (binary.substring(4, 5) == "1") {
        decimal = decimal + 8
    }
    if (binary.substring(3, 4) == "1") {
        decimal = decimal + 16
    }
    if (binary.substring(2, 3) == "1") {
        decimal = decimal + 32
    }
    if (binary.substring(1, 2) == "1") {
        decimal = decimal + 64
    }
    if (binary.substring(0, 1) == "1") {
        decimal = decimal + 128
    }
    return (decimal);
}



Validator.isEmpty = function (n) {
    return (n === null || n === undefined || n === false || ((n+"").trim() === ""));
}


Validator.isEqual = function (n1, n2) {
    if (Validator.isEmpty(n1) || Validator.isEmpty(n2)) {
        return false;
    }
    return n1 == n2;
}





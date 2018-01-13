#!/usr/bin/env nodejs

function print_usage() {
	console.log ('Usage:');
	console.log ('mldock_installed_packages.mp spec');
}

var CLP=new CLParams(process.argv);

var arg1=CLP.unnamedParameters[0]||'';

if (!arg1) {
	print_usage();
	return;
}

mkdir_if_needed(require('os').homedir()+'/.mountainlab')
var mldock_config_path=require('os').homedir()+'/.mountainlab/mldock';
mkdir_if_needed(mldock_config_path);

if (arg1=='spec') {
	var json;
	try {
		json=require('fs').readFileSync(mldock_config_path+'/mldock.spec');
	}
	catch(err) {
		json='';
	}
	if (!json) {
		var a={error:'File is empty or missing: mldock.spec',processors:[]};
		console.log (JSON.stringify(a,null,4));
		return;
	}
	var obj=try_parse_json(json);
	if (!obj) {
		var a={error:'Unable to parse json in mldock.spec',processors:[]};
		console.log (JSON.stringify(a,null,4));
		return;
	}
	console.log (JSON.stringify(obj,null,4));
}

function try_parse_json(txt) {
	try {
		return JSON.parse(txt);
	}
	catch(err) {
		return null;
	}
}

function CLParams(argv) {
	this.unnamedParameters=[];
	this.namedParameters={};

	var args=argv.slice(2);
	for (var i=0; i<args.length; i++) {
		var arg0=args[i];
		if (arg0.indexOf('--')===0) {
			arg0=arg0.slice(2);
			var ind=arg0.indexOf('=');
			if (ind>=0) {
				this.namedParameters[arg0.slice(0,ind)]=arg0.slice(ind+1);
			}
			else {
				//this.namedParameters[arg0]=args[i+1]||'';
				//i++;
				this.namedParameters[arg0]='';
			}
		}
		else if (arg0.indexOf('-')===0) {
			arg0=arg0.slice(1);
			this.namedParameters[arg0]='';
		}
		else {
			this.unnamedParameters.push(arg0);
		}
	}
};

function mkdir_if_needed(path) {
  try {
    fs.mkdirSync(path);
  }
  catch(err) {
  }
}
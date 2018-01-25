#!/usr/bin/env nodejs

var fs=require('fs');

function print_usage() {
	console.log ('Usage:');
	console.log ('mldock_update spec');
	console.log ('mldock_update update');
}

mkdir_if_needed(require('os').homedir()+'/.mountainlab')
var mldock_config_path=require('os').homedir()+'/.mountainlab/mldock';
mkdir_if_needed(mldock_config_path);

var CLP=new CLParams(process.argv);

var arg1=CLP.unnamedParameters[0]||'';

if (!arg1) {
	print_usage();
	return;
}

if (arg1=='spec') {
	var spec={
		processors:[
			{
				name:'mldock.update',
				version:'0.1',
				inputs:[],
				outputs:[],
				parameters:[],
				opts:{
					force_run:true
				},
				exe_command:__filename+' update'
			}
		]
	}
	console.log (JSON.stringify(spec,null,4));
	return;
}
else if (arg1=='update') {
	var config_fname=mldock_config_path+'/mldock_config.json';
	do_update(config_fname,function(err) {
		if (err) {
			console.error(err);
			process.exit(-1);
			return;
		}
	});
}
else {
	console.error('Unrecognized command: '+arg1);
	process.exit(-1);
	return;
}

function do_update(config_fname,callback) {
	if (!require('fs').existsSync(config_fname)) {
		callback('File does not exist: '+config_fname);
		return;
	}
	var json=require('fs').readFileSync(config_fname,'utf-8');
	var obj=try_parse_json(json);
	if (!obj) {
		callback('Error parsing json in '+config_fname);
		return;
	}

	var packages=obj.packages||[];
	if (packages.length==0) {
		console.log ('No packages found in configuration file.');
		return;
	}
	var spec={processors:[]};
	foreach_async(packages,function(i,package,cb) {
		install_package(package,function(err,package_spec) {
			if (err) {
				callback(err);
				return;
			}
			var package_processors=package_spec.processors||[];
			console.log (`Adding ${package_processors.length} processors from package ${package.name}`);
			for (var i in package_processors) {
				var pp=package_processors[i];
				pp.package_name=package.name;
				spec.processors.push(pp);
			}
			cb();
		});
	},function() {
		console.log ('Writing spec...');
		require('fs').writeFileSync(mldock_config_path+'/mldock.spec',JSON.stringify(spec,null,4),'utf-8');
		console.log ('Done.');
		callback(null);
	});
}

function install_package(package,callback) {
	console.log (`Installing package: ${package.name} ...`);
	if (!package.git_url) {
		callback('Missing field in package config: git_url');
		return;
	}
	system_call('docker build -t mldock/'+package.name+' '+package.git_url,{print_console:true},function(err,tmp) {
		if (err) {
			console.error(`Error building docker image for package ${package.name}: `+err);
			return;
		}
		read_package_spec_from_container('mldock/'+package.name,package.git_url,package.prefix,function(err,spec) {
			if (err) {
				console.error(`Error reading spec from container for package ${package.name}: `+err);
				return;
			}
			callback('',spec);
		})
	});
}

function read_package_spec_from_container(image_tag,package_url,prefix,callback) {
	var aaa="echo '[' ; for i in *.spec; do cat \\$i; echo ','; done ; echo '{}]'";
	var cmd2='docker run -t '+image_tag+' /bin/bash -c "'+aaa+'"';
	system_call(cmd2,{print_console:false},function(err,tmp2) {
		if (err) {
			callback(err);
			return;
		}
		var processors=[];
		var json=tmp2.console_out;
		var specs=JSON.parse(json);
		for (var j=0; j<specs.length; j++) {
			if (specs[j].processors) {
				for (var k=0; k<specs[j].processors.length; k++) {
					var pp=specs[j].processors[k];
					if (prefix)
						pp.name=prefix+'.'+pp.name;
					pp=adjust_processor(pp,image_tag,package_url);
					processors.push(pp);
				}
			}
		}
		callback('',{processors:processors});
	});
}

function adjust_processor(pp,image_tag,package_url) {
	var pp2=JSON.parse(JSON.stringify(pp));
	pp2.docker_image_tag=image_tag;
	pp2.exe_command_within_container=pp.exe_command;
	pp2.exe_command=__dirname+'/mldock_run '+pp.name+' $(arguments)';
	pp2.package_url=package_url;
	return pp2;
}

function system_call(cmd,opts,callback) {
	if (opts.print_console)
		console.log ('RUNNING: '+cmd);

	var pp=require('child_process').exec(cmd);
	pp.stdout.setEncoding('utf8');
	pp.stderr.setEncoding('utf8');
	var console_out='';
	pp.on('close', function(code,data) {
		if (data) console_out+=data;
		if (callback) callback('',{console_out:console_out,exit_code:code});
		callback=0;
	});
	/*
	//important not to do the following, because we can miss the console output
	pp.on('exit', function(code,data) {
		if (data) console_out+=data;
		if (callback) callback({success:true,console_out:console_out,exit_code:code});
		callback=0;
	});
	*/
	pp.on('error',function(err) {
		if (callback) callback(err.message,{console_out:console_out,exit_code:-1});
		callback=0;
	});
	pp.stdout.on('data',function(data) {
		if (opts.print_console)
			console.log (data.trim());
		console_out+=data;
	});
	pp.stderr.on('data',function(data) {
		if (opts.print_console)
			console.log (data.trim());
		console_out+=data;
	});
};


function foreach_async(list,step,callback) {
	var ii=0;
	do_step();
	function do_step() {
		if (ii>=list.length) {
			callback();
			return;
		}
		step(ii,list[ii],function() {
			ii++;
			do_step();
		});
	}
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

function make_random_id(len) {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for( var i=0; i < len; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}

function mkdir_if_needed(path) {
  try {
    fs.mkdirSync(path);
  }
  catch(err) {
  }
}
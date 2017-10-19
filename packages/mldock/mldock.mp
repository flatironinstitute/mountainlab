#!/usr/bin/env nodejs

var fs=require('fs');

var CLP=new CLParams(process.argv);

var arg1=CLP.unnamedParameters[0]||'';

if (arg1=='update') {
	update(function() {
	});
}
else if (arg1=='install') {
	var url=CLP.unnamedParameters[1]||'';
	var name=CLP.unnamedParameters[2]||'';
	if ((!url)||(!name))
		throw new Error('Usage: mldock.mp install [url] [name]');
	install(url,name,function() {
		update(function() {

		});
	});
}
else if (arg1=='spec') {
	var spec=read_spec();
	console.log (JSON.stringify(spec,null,4));
}
else {
	var processor_name=arg1;
	var spec=read_spec();
	if (!tmp.success) {
		throw new Error(tmp.error);
	}
	var spec=tmp.spec;
	var pp=find_processor_from_spec(spec,processor_name);
	if (!pp) {
		throw new Error('Processor not found: '+processor_name);
	}
	create_working_directory(function(working_path) {
		fs.mkdirSync(working_path+'/outputs');
		var docker_options='-v '+working_path+'/outputs:/working/outputs';
		try {
			var args={};
			for (var ii in pp.inputs) {
				var input0=pp.inputs[ii];
				var ikey=input0.name;
				if (ikey in CLP.namedParameters) {
					docker_options+=' -v '+absolute_path(CLP.namedParameters[ikey])+':/working/inputs/input_'+ikey;
					args[ikey]='/working/inputs/input_'+ikey;
				}
				else {
					if (!input0.optional) {
						throw new Error('Missing required input: '+ikey);
					}
				}
			}
			for (var ii in pp.outputs) {
				var output0=pp.outputs[ii];
				var okey=output0.name;
				if (okey in CLP.namedParameters) {
					var path0=working_path+'/output_'+okey;
					args[okey]='/working/outputs/output_'+okey;
				}
				else {
					if (!output0.optional) {
						throw new Error('Missing required output: '+okey);
					}
				}
			}
			for (var ii in pp.parameters) {
				var param0=pp.parameters[ii];
				var pkey=param0.name;
				if (pkey in CLP.namedParameters) {
					args[pkey]=CLP.namedParameters[pkey];
				}
				else {
					if (!param0.optional) {
						throw new Error('Missing required parameter: '+pkey);
					}
				}
			}
			var exe_cmd=pp.exe_command_within_container;
			var arguments_str=make_arguments_str(args);
			exe_cmd=exe_cmd.split('$(arguments)').join(arguments_str);

			var uid=process.getuid();
			exe_cmd+=' && chown -R '+uid+':'+uid+' /working/outputs';
			run_docker_container(pp.container_name,docker_options,exe_cmd,{print_console:true},function(aa) {
				try {
					if (aa.exit_code!==0) {
						throw new Error(aa.exit_code);
					}
					for (var ii in pp.outputs) {
						var output0=pp.outputs[ii];
						var okey=output0.name;
						if (okey in CLP.namedParameters) {
							var path0=working_path+'/outputs/output_'+okey;
							console.log ('Moving '+path0+' -> '+CLP.namedParameters[okey]);
							move_file_sync(path0,CLP.namedParameters[okey]);
						}
					}
					cleanup(function(tmp) {
						if (!tmp.success) {
							console.error('Error cleaning up');
							process.exit(-1);
						}
					});
				}
				catch(err2) {
					console.error (err2.stack);
					console.error(err2.message);
					cleanup(function(tmp) {
						if (!tmp.success) console.error(tmp.error);
						process.exit(-1);
					});		
				}
			});
		}
		catch(err) {
			console.error (err.stack);
			console.error(err.message);
			cleanup(function(tmp) {
				if (!tmp.success) console.error(tmp.error);
				process.exit(-1);
			});
			
		}

		function cleanup(callback) {
			run_docker_container(pp.container_name,docker_options,'touch /working/outputs/dummy.txt ; rm /working/outputs/*',{print_console:false},function(aa) {
				require('child_process').execSync('rmdir '+working_path+'/outputs');
				require('child_process').execSync('rmdir '+working_path);
				callback({success:true});
			});
		}
	});
}

function read_spec() {
	var txt=fs.readFileSync(__dirname+'/mldock.spec','utf-8')||'{processors:[]}';
	var spec=JSON.parse(txt);
	return spec;
}

function absolute_path(path) {
	return require('path').resolve(path);
}

function copy_file_sync(src,dst) {
	if (fs.existsSync(dst))
		require('child_process').execSync('rm '+dst);
	require('child_process').execSync('cp '+src+' '+dst);
}

function move_file_sync(src,dst) {
	if (fs.existsSync(dst))
		require('child_process').execSync('rm '+dst);
	require('child_process').execSync('mv '+src+' '+dst);
}

function create_working_directory(callback) {
	system_call('mlconfig tmp',{},function(tmp) {
		if (!tmp.success) {
			throw new Error("Error in system call: mlconfig tmp");
		}
		var tmppath=tmp.console_out.trim();
		var path=tmppath+'/dockerpack_working_'+make_random_id(6);
		fs.mkdirSync(path);
		callback(path);
	});
}

function make_arguments_str(args) {
	var list=[];
	for (var key in args) {
		list.push('--'+key+'='+args[key]);
	}
	return list.join(' ');
}

function find_processor_from_spec(spec,pname) {
	var processors=spec.processors||[];
	for (var i in processors) {
		if (processors[i].name==pname)
			return processors[i];
	}
	return null;
}

function update(callback) {
	var cmd='docker images mldock/*';
	system_call(cmd,{print_console:false},function(tmp) {
		if (!tmp.success) {
			throw new Error(tmp.error);
		}
		var str=tmp.console_out;
		var lines=str.split('\n');
		var images=[];
		for (var i=1; i<lines.length; i++) {
			var name=lines[i].split(' ')[0]||'';
			if (name) images.push(name);
		}
		var processors=[];
		foreach(images,{num_parallel:1},function(ii,image,cb) {
			console.log (image);
			var aaa="echo '[' ; for i in *.spec; do cat \\$i; echo ','; done ; echo '{}]'";
			var cmd2='docker run -t '+image+' /bin/bash -c "'+aaa+'"';
			system_call(cmd2,{print_console:false},function(tmp2) {
				if (!tmp2.success) {
					throw new Error('Error running docker image '+image+': '+tmp2.error);
				}
				var json=tmp2.console_out;
				var specs=JSON.parse(json);
				for (var j=0; j<specs.length; j++) {
					if (specs[j].processors) {
						for (var k=0; k<specs[j].processors.length; k++) {
							var pp=specs[j].processors[k];
							pp=adjust_processor(pp,image);
							processors.push(pp);
						}
					}
				}
				cb();
			});
		},function() {
			var spec={};
			spec.processors=processors;
			console.log ('Writing spec for '+processors.length+' processors to '+__dirname+'/mldock.spec');
			fs.writeFileSync(__dirname+'/mldock.spec',JSON.stringify(spec,null,4),'utf-8');
			callback();
		});
	})
}

function adjust_processor(pp,image) {
	var pp2=JSON.parse(JSON.stringify(pp));
	pp2.docker_image=image;
	pp2.exe_command_within_container=pp.exe_command;
	pp2.exe_command=__filename+' '+pp.name+' $(arguments)';
	return pp2;
}

function get_spec(callback) {
	var spec={processors:[]};
	var path0=__dirname+'/../../dockerpackages';
	var list=fs.readdirSync(path0);
	foreach(list,{},function(i,package_name,cb) {
		var path1=path0+'/'+package_name;
		var fname=path1+'/dockerpackage.specs';
		if (fs.existsSync(fname)) {
			var specs=read_json_file(fname);
			for (var i=0; i<specs.length; i++) {
				var spec0=specs[i].spec;
				for (var j=0; j<spec0.processors.length; j++) {
					var pp=spec0.processors[j];
					pp.package_name=package_name;
					pp.name='dp.'+pp.name;
					pp.mp_file=specs[i].mp_file;
					pp.exe_command_within_container=pp.exe_command;
					pp.exe_command=__filename+' '+pp.name+' $(arguments)';
					spec.processors.push(pp);
				}
			}
		}
		cb();
	},function() {
		callback({success:true,spec:spec});
	});
}

function find_mp_files_in_package(package_path) {
	var ret=[];
	var fs=require('fs');
	var list=fs.readdirSync(package_path);
	for (var i=0; i<list.length; i++) {
		var stat=fs.statSync(package_path+'/'+list[i]);
		if (stat.isFile()) {
			if (ends_with(list[i],'.mp')) {
				ret.push(list[i]);
			}
		}
		else if (stat.isDirectory()) {
			var ret2=find_mp_files_in_package(package_path+'/'+list[i]);
			for (var j=0; j<ret2.length; j++) {
				ret.push(list[i]+'/'+ret2[j]);
			}
		}
	}
	return ret;
}

function ends_with(str,str2) {
	return (str.slice(str.length-str2.length,str.length)==str2);
}

function install(url,name,callback) {
	system_call('docker build -t mldock/'+name+' '+url,{print_console:true},function(tmp) {
		callback(tmp);
	});
}

function run_docker_container(image_name,docker_options,cmd,opts,callback) {
	system_call('docker run -t '+docker_options+' '+image_name+' /bin/bash -c \"'+cmd+'\"',{print_console:opts.print_console},function(tmp) {
		callback(tmp);
	});
}

function read_json_file(fname) {
	return JSON.parse(fs.readFileSync(fname,'utf-8'));
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

function foreach(array,opts,step_function,end_function) {
	var num_parallel=opts.num_parallel||1;
	var num_running=0;
	var num_finished=0;
	var already_called_end=false;
	var ii=0;
	next_step();
	function next_step() {
		if (num_finished>=array.length) {
			setTimeout(function() { //important to do it this way so we don't accumulate a call stack
				if (!already_called_end) { 
					already_called_end=true;
					end_function();
				}
			},0);
			return;
		}
		while ((ii<array.length)&&(num_running<num_parallel)) {
			num_running++;
			ii++;
			step_function(ii-1,array[ii-1],function() {
				num_running--;
				num_finished++;
				setTimeout(function() { //important to do it this way so we don't accumulate a call stack
					next_step();
				},0);
			});
		}
	}
}

function system_call(cmd,opts,callback) {
	if (opts.print_console)
		console.log ('RUNNING: '+cmd);

	var pp=require('child_process').exec(cmd);
	pp.stdout.setEncoding('utf8');
	pp.stderr.setEncoding('utf8');
	var console_out='';
	pp.on('close', function(code) {
		if (callback) callback({success:true,console_out:console_out,exit_code:code});
		callback=0;
	});
	pp.on('exit', function(code) {
		if (callback) callback({success:true,console_out:console_out,exit_code:code});
		callback=0;
	});
	pp.on('error',function(err) {
		if (callback) callback({success:false,error:err.message,console_out:console_out,exit_code:-1});
		callback=0;
	});
	pp.stdout.on('data',function(data) {
		if (opts.print_console)
			console.log (data);
		console_out+=data;
	});
	pp.stderr.on('data',function(data) {
		if (opts.print_console)
			console.log (data);
		console_out+=data;
	});
};

function make_random_id(len) {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for( var i=0; i < len; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}
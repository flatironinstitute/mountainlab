exports.mpdock_exec=mpdock_exec;

function mpdock_exec(processor_name,package_spec,CLP_args,callback) {
	var spec=package_spec;
	var pp=find_processor_from_spec(spec,processor_name);
	if (!pp) {
		callback('Processor not found: '+processor_name);
		return;
	}
	create_working_directory(function(err000,working_path) {
		if (err000) {
			callback(err000);
			return;
		}
		require('fs').mkdirSync(working_path+'/outputs');
		var docker_options='-v '+working_path+'/outputs:/working/outputs';
		try {
			var args={};
			for (var ii in pp.inputs) {
				var input0=pp.inputs[ii];
				var ikey=input0.name;
				if (ikey in CLP_args) {
					var ext=get_file_extension(CLP_args[ikey]);
					docker_options+=' -v '+absolute_path(CLP_args[ikey])+':/working/inputs/input_'+ikey+'.'+ext;
					args[ikey]='/working/inputs/input_'+ikey+'.'+ext;
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
				if (okey in CLP_args) {
					var ext=get_file_extension(CLP_args[okey]);
					var path0=working_path+'/output_'+okey+'.'+ext;
					args[okey]='/working/outputs/output_'+okey+'.'+ext;
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
				if (pkey in CLP_args) {
					args[pkey]=CLP_args[pkey];
				}
				else {
					if (!param0.optional) {
						throw new Error('Missing required parameter: '+pkey);
					}
					args[pkey]=param0.default_value||'';
				}
			}
			var exe_cmd=pp.exe_command_within_container;
			var arguments_str=make_arguments_str(args);
			arguments_str+=' --_tempdir=/tmp'; //note: this will be the /tmp directory inside the container
			exe_cmd=exe_cmd.split('$(arguments)').join(arguments_str);
			for (var argname in args) {
				exe_cmd=exe_cmd.split('$'+argname+'$').join(args[argname]);
			}
			//exe_cmd=exe_cmd.split('(').join('\\(');
			//exe_cmd=exe_cmd.split(')').join('\\)');
			//exe_cmd=exe_cmd.split("'").join("\\'");
			exe_cmd=exe_cmd.split('"').join('\\"');

			var uid=process.getuid();
			exe_cmd+=' && chown -R '+uid+':'+uid+' /working/outputs';
			run_docker_container(pp.docker_image_tag,docker_options,exe_cmd,{print_console:true},function(aa_err,aa) {
				if (aa_err) {
					console.error('Error running docker container: '+aa_err);
					cleanup(function(tmp) {
						if (!tmp.success) {
							console.error('Error cleaning up');
							process.exit(-1);
						}
					});
					return;
				}
				try {
					if (aa.exit_code!==0) {
						throw new Error(aa.exit_code);
					}
					for (var ii in pp.outputs) {
						var output0=pp.outputs[ii];
						var okey=output0.name;
						if (okey in CLP_args) {
							var ext=get_file_extension(CLP_args[okey]);
							var path0=working_path+'/outputs/output_'+okey+'.'+ext;
							console.log ('Moving '+path0+' -> '+CLP_args[okey]);
							move_file_sync(path0,CLP_args[okey]);
						}
					}
					cleanup(function(tmp) {
						if (!tmp.success) {
							console.error('Error cleaning up');
						}
						callback(null);
						return;
					});
				}
				catch(err2) {
					console.error (err2.stack);
					console.error(err2.message);
					cleanup(function(tmp) {
						if (!tmp.success) {
							console.error(tmp.error);
						}
						callback(err2.message);
						return;
					});		
				}
			});
		}
		catch(err) {
			console.error (err.stack);
			console.error(err.message);
			cleanup(function(tmp) {
				if (!tmp.success) console.error(tmp.error);
				callback(err.message);
				return;
			});	
		}

		function cleanup(callback) {
			run_docker_container(pp.docker_image_tag,docker_options,'touch /working/outputs/dummy.txt ; rm /working/outputs/*',{print_console:false},function(aa) {
				require('child_process').execSync('rmdir '+working_path+'/outputs');
				require('child_process').execSync('rmdir '+working_path);
				callback({success:true});
			});
		}
	});
}

function find_processor_from_spec(spec,pname) {
	var processors=spec.processors||[];
	for (var i in processors) {
		if (processors[i].name==pname)
			return processors[i];
	}
	return null;
}

function create_working_directory(callback) {
	system_call('mlconfig tmp',{print_console:false},function(err,tmp) {
		if (err) {
			callback(err);
			return;
		}
		var tmppath=tmp.console_out.trim();
		var path=tmppath+'/mpdock_working_'+make_random_id(6);
		require('fs').mkdirSync(path);
		callback(null,path);
		// TODO: clean up old mpdock_working_* directories that may not have gotten removed, despite our very best efforts
	});
}

function get_file_extension(path) {
	var list=path.split('/');
	var fname=list[list.length-1];
	var list2=fname.split('.');
	if (list2.length<=1) return 'dat';
	return list2[list2.length-1];
}

function absolute_path(path) {
	return require('path').resolve(path);
}

function make_arguments_str(args) {
	var list=[];
	for (var key in args) {
		list.push('--'+key+'='+args[key]);
	}
	return list.join(' ');
}

function run_docker_container(image_tag,docker_options,cmd,opts,callback) {
	system_call('docker run -t '+docker_options+' '+image_tag+' /bin/bash -c \"'+cmd+'\"',{print_console:opts.print_console},function(err,tmp) {
		callback(err,tmp);
	});
}

function move_file_sync(src,dst) {
	if (require('fs').existsSync(dst))
		require('child_process').execSync('rm '+dst);
	require('child_process').execSync('mv '+src+' '+dst);
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

function make_random_id(len) {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for( var i=0; i < len; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}

function mkdir_if_needed(path) {
  try {
    require('fs').mkdirSync(path);
  }
  catch(err) {
  }
}

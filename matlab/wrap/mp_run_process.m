function mp_run_process(processor_name,inputs,outputs,params,opts)

%% Example: mp_run_process('mountainsort.bandpass_filter',struct('timeseries','tetrode6.mda'),struct('timeseries_out','tetrode6_filt.mda'),struct('samplerate',32556,'freq_min',300,'freq_max',6000));

if nargin<4, params=struct; end;
if nargin<5, opts=struct; end;
if ~isfield(opts,'mp_command') opts.mp_command='mp-run-process'; end;

cmd=opts.mp_command;
cmd=[cmd,' ',processor_name];
cmd=[cmd,' ',create_arg_string(inputs)];
cmd=[cmd,' ',create_arg_string(outputs)];
cmd=[cmd,' ',create_arg_string(params)];
return_code=system_call(cmd);
if (return_code~=0)
    error('Error running processor: %s',processor_name);
end;

function str=create_arg_string(params)
list={};
keys=fieldnames(params);
for i=1:length(keys)
    key=keys{i};
    val=params.(key);
    if (iscell(val))
        for cc=1:length(val)
            list{end+1}=sprintf('--%s=%s',key,create_val_string(val{cc}));
        end;
    else
        list{end+1}=sprintf('--%s=%s',key,create_val_string(val));
    end;
end;
str=strjoin(list,' ');

function str=create_val_string(val)
str=num2str(val);

function return_code=system_call(cmd)
cmd=sprintf('LD_LIBRARY_PATH=/usr/local/lib %s',cmd);
return_code=system(cmd);
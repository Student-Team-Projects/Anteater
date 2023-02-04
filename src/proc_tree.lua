description = "Proc_tree"
short_description = "Process tree chisel"
category = "I/O"

args =
{
    {
        name = "root_pid",
        description = "pid of the root process",
        argtype = "int",
        optional = false
    },
    {
        name = "verbose",
        description = "flag indicating verbose mode",
        argtype = "boolean",
        optional = true
    }
}

root_pid = nil
verbose = false
tree = {}

-- this is a callback for setting command line arguments
function on_set_arg(name, value)
    if name == "root_pid" then
        root_pid = tonumber(value)
        tree[root_pid] = {}
    elseif name == "verbose" and value == "true" then
        verbose = true
    end
    return true
end

-- execl, execlp, execle, execv, execvp, execvpe are front-ends for execve (see man exec)
evt_types = {
    "execve",
    "write",
    "fork",
    "clone",
    "procexit"
}

-- fields as explained here:
-- https://github.com/annulen/sysdig-wiki/blob/master/Sysdig-User-Guide.md
evt_fields = {
    "evt.args",
    "evt.dir",
    "evt.rawarg.data",
    "evt.rawres",
    "evt.rawtime",
    "evt.type",
    "fd.num",
    "proc.args",
    "proc.name",
    "proc.pid",
    "proc.ppid"
}

CHISEL_FIELD_PREFIX = "CHISEL_FIELD_HANDLE_"

function set_fields()
    for _, field_name in pairs(evt_fields) do
        _G[CHISEL_FIELD_PREFIX .. field_name] = chisel.request_field(field_name)
    end
end

function get_fields()
    for _, field_name in pairs(evt_fields) do
        local var_name = string.gsub(field_name, "%.", "_")
        _G[var_name] = evt.field(_G[CHISEL_FIELD_PREFIX .. field_name])
    end
end

function on_init()
    set_fields()
    local filter = "evt.type in (" .. table.concat(evt_types, ",") .. ")"
    if verbose then
        io.stderr:write("setting filter '" .. filter .. "'\n")
    end
    chisel.set_filter(filter)
    return true
end

function on_capture_start()
    if not sysdig.is_live() then
        return true
    end
    local status = os.execute("kill -USR1 " .. root_pid)
    if verbose then
        io.stderr:write("kill result: " .. status .. "\n")
    end
    return status == 0
end

function on_capture_end()
    if verbose then
        print("===============ROOT PROCESS EXECUTION FINISHED===============")
        print("===PRINTING THE TREE OF ALL PROCESSES DURING THE EXECUTION===")

        print("ROOT PID: " .. root_pid)

        local count = 0
        for pid, child_pids in pairs(tree) do
            print(pid .. ": [" .. table.concat(child_pids, ", ") .. "]")
            count = count + 1
        end
        print("TOTAL NUMBER OF PROCESSES: " .. count)
    end
end

local function hexencode(str)
    if str == nil or str == "nil" or str == "" then
        return ""
    end
    return (str:gsub(".", function(char) return string.format("\\x%2x", char:byte()) end))
 end

function on_event()
    -- sets variables for event fields e.g.
    -- proc_name = evt.field(_G["CHISEL_FIELD_proc.name"])
    get_fields()

    -- check if this is maybe a new process in root subtree
    if tree[proc_ppid] and not tree[proc_pid] then
        tree[proc_pid] = {}
        table.insert(tree[proc_ppid], proc_pid)
    end

    -- this is not in the root subtree
    if not tree[proc_pid] then
        return false
    end

    print(
        "type=" .. string.upper(evt_type)
        .. " pid=" .. (proc_pid or "nil")
        .. " ppid=" .. (proc_ppid or "nil")
        .. " name=" .. hexencode(tostring(proc_name))
        .. " res=" .. (evt_rawres or "nil")
        .. " data=" .. hexencode(tostring(evt_rawarg_data))
	.. " fd=" .. (fd_num or "nil")
        .. " time=" .. evt_rawtime
    )
    
    if evt_type == "procexit" and proc_pid == root_pid then
        sysdig.end_capture()
    end

    return true
end

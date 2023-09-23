
-- 性能指标收集器
Profiler = {
	tempBeginTick = 0,
	totalCpuCost = 0,
	reports = {},
	reportsByTitle = {},
	getTickCount = function()
		return os.clock() * 1000
	end
}
function Profiler:start()
	local function MakeReport(info)
		local name = info.name or 'anonymous'
		local title = string.format("%s: %s: %s", name, info.source, info.linedefined)
		local report = self.reportsByTitle[title]
		if not report then
			report = {}
			report.title = title
			report.callCount = 0
			report.cpuCost = 0
			self.reportsByTitle[title] = report
			table.insert(self.reports, report)
		end
		return report
	end
	local function ProfilingHandler(event)
		local info = debug.getinfo(2, 'nS')
		if event == "call" then
			local report = MakeReport(info)
			report.tempBeginTick = self:getTickCount()
			report.callCount = report.callCount + 1
		elseif event == "return" then
			local endTick = self:getTickCount()
			local report = MakeReport(info)
			if report.tempBeginTick then
				report.cpuCost = report.cpuCost + (endTick - report.tempBeginTick)
				report.tempBeginTick = nil
			end
		end
	end
    debug.sethook(ProfilingHandler, 'cr', 0)
    self.tempBeginTick = self:getTickCount()
end
function Profiler:stop()
    self.totalCpuCost = self.totalCpuCost + (self:getTickCount() - self.tempBeginTick)
    debug.sethook()
    table.sort(self.reports, function(lt, rt)
        return lt.cpuCost > rt.cpuCost
    end)
end
function Profiler:dump(num)
	if self.totalCpuCost <= 0 then
		return
	end
	if table.getn(self.reports) < num then
		num = table.getn(self.reports)
	end
	for idx = 1, num do
		local report = self.reports[idx]
		local percent = report.cpuCost / self.totalCpuCost * 100
		if percent < 1 then
			break
		end
		local messages = {}
		table.insert(messages, "CostCpu：" ..  string.format("%.2f", report.cpuCost))
		table.insert(messages, "Percent：" .. string.format("%.2f", percent))
		table.insert(messages, "CallCount：" .. report.callCount)
		table.insert(messages, "Average：" .. string.format("%.2f", report.cpuCost / report.callCount))
		table.insert(messages, "Title：" .. report.title)
		print("【性能指标】：" .. table.concat(messages, ", "))
	end
end

-- 函数包装器，用于捕获报错和超时
FunctionWrapper = {}
function FunctionWrapper:new(quota, errorDump, timeoutDump)
	if not quota then
		quota = 16
	end
	if not errorDump then
		errorDump = logger_error
	end
	if not timeoutDump then
		timeoutDump = logger_warn
	end
	local obj = {}
	obj.quota = quota
	obj.errorDump = errorDump
	obj.timeoutDump = timeoutDump
	return setmetatable(obj, {__index = self})
end
function FunctionWrapper:exec(func, ...)
	local beginTick = nil
	if self.quota then
		beginTick = gettickcount()
	end
	Profiler:start()
	local traceMsg = ""
	local results = {xpcall(func, function(err)
		traceMsg = "<" .. err .. "> " .. debug.traceback()
	end, ...)}
	Profiler:stop()
	if not results[1] then
		self.errorDump("【报错信息】：" .. traceMsg)
	end
	if beginTick then
		local cpuCost = gettickcount() - beginTick
		if cpuCost >= self.quota then
			local info = debug.getinfo(func, "S")
			local messages = {}
			table.insert(messages, "Quota：" .. self.quota)
			table.insert(messages, "Cost：" .. cpuCost)
			table.insert(messages, "Line：" .. info.linedefined)
			table.insert(messages, "Path：" .. info.source)
			self.timeoutDump("【超时信息】：" .. table.concat(messages, ", "))
		end
	end
	table.remove(results, 1)
	return unpack(results)
end
function FunctionWrapper:ExecuteLuaStr(funcName, ...)
	self:new(10):exec(_G[funcName], 1, ...)
end

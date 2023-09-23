print("===========")
print(package.path)
require "logic"

function BeforeLoadScript()
    print("BeforeLoadScript")
end

function PrintDump()
    print("PrintDump")
end

function AfterLoadScript()
    print("BeforeLoadScript")
end

local count = 1
for k, v in pairs(package) do
    print(k)
    count = count + 1
end

for name, v in pairs(package.loaded) do
    local path = package.searchpath(name, package.path)
    if path then
        print(name, v, path)
        dofile(path)
        
        --package.preload[name] = function(moduleName)
        --    print("预加载：" .. moduleName)
        --    local env = {}
        --    return env
        --end
        --require(name)
    end
end
print(package.config)
print("dofile success! count:" .. count)

for k, v in pairs(package.searchers) do
    print(k)
end

print("-------")
for k, v in pairs(package.preload) do
    print(k)
end
print("######################################")


CheckAddBuffManager = {
	typeYiHuo = "YiHuo",
	typeGuildEscort = "GuildEscort",
	handles = {},
    myFunction = function(x, y)
        print(x .. "=========".. y)
        return x + y
    end
}
function CheckAddBuffManager:Register(tope, handle)
	self.handles[tope] = handle
end
function CheckAddBuffManager:Check(pCret, buffDb)
    print("开始check-111---------" .. pCret .. "==" .. buffDb)
	for _, handle in ipairs(self.handles) do
		if not handle(pCret, buffDb) then
			return false
		end
	end
	return true
end

function CheckAddBuffManagerCheck(pCret, buffDb)
    print("开始check----------" .. pCret .. "==" .. buffDb)
end

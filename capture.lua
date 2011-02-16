#!/usr/bin/env lua

require "v4l"
require "table"

function saveimg(img)
 file = io.open("image.ppm", "w+")
 file:write("P3\n".. w .. " " .. h .."\n255\n") -- RGB IMAGE
 
 for i,v in ipairs(img) do
    local p = v .. "\n"  
    file:write(p)
  end
 
  file:close()
end



function sleep(length)
  local start = os.clock()
  while os.clock() - start < length do end
end

camera = #arg

if camera < 1 then
 camera = "/dev/video0"
else
 camera = arg[1]
end

print(camera)

dev = v4l.open(camera)
w, h = v4l.widht(), v4l.height()

print(w .. "x" .. h)

a = v4l.getframe()

saveimg(a)
table.save(a, "teste")

a = nil 
v4l.close(dev);




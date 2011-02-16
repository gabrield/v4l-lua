#!/usr/bin/env lua

require "v4l"
require "table"

function saveimg(img)
 file = io.open("image.ppm", "w+")
 file:write("P3\n".. w .. " " .. h .."\n255\n") -- RGB IMAGE
 
 for i=0,#img do
    local p = a[i] .. "\n"  
    file:write(p)
  end
 
  file:close()
end

camera = #arg

if camera < 1 then
 camera = "/dev/video0"
else
 camera = arg[1]
end

print(camera)

dev = v4l.open(camera)

if dev < 0 then
 print("camera not found")
 os.exit(0)
end

w, h = v4l.widht(), v4l.height()

print(w .. "x" .. h)

for i=0,10 do
 a = v4l.getframe()
end

saveimg(a)
a = nil


v4l.close(dev);




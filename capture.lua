#!/usr/bin/env lua

local camera = arg[1] or "/dev/video0"
local dev = require "v4l".open(camera)

local function saveimg(img, w, h)
 file = io.open("image.ppm", "w+")
 file:write("P3\n".. w .. " " .. h .."\n255\n") -- RGB IMAGE
 for i = 1, #img do
    local p = img[i] .. "\n"
    file:write(p)
  end
  file:close()
end

local w, h = dev:width(), dev:height()

print(camera .. ": " ..w .. "x" .. h)

local a
for i = 1, 10 do -- take 10 pics to get a better image
   a = dev:getframe()
-- print(a[i] .. " " .. a[i+1] .. " "..  a[i+2])
end

saveimg(a, w, h)

local fd = dev:fd()
dev:close()
print("File descriptor closed: " .. fd)

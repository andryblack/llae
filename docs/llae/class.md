# Class Module

The `class` module provides a simple yet powerful object-oriented programming (OOP) implementation for Lua. It supports inheritance, instance creation, and type checking.

## Usage

The module returns a single function that creates a new class. This function can be used in two ways:

```lua
local class = require 'llae.class'

-- Create a class without inheritance
local MyClass = class()

-- Create a class with inheritance
local MyDerivedClass = class(MyClass)

-- Create a class with a name (useful for type checking)
local Animal = class(nil)
```

## Class Creation

### Parameters
- `base` (table, optional): Base class to inherit from. Defaults to an empty table.

### Returns
- (table): A new class table with the following features:
  - Constructor function (`new`)
  - Inheritance tracking (`is_a`)
  - Base class reference (`baseclass`)

## Features

### Constructor
Classes can define an initializer method `_init` that is automatically called when creating new instances:

```lua
local Point = class()

function Point:_init(x, y)
  self.x = x
  self.y = y
end

local p = Point.new(10, 20)
```

### Inheritance
Classes support single inheritance with automatic method copying:

```lua
local Shape = class()

function Shape:_init(color)
  self.color = color
end

function Shape:getColor()
  return self.color
end

local Circle = class(Shape)

function Circle:_init(color, radius)
  Circle.baseclass._init(self, color)  -- Call base class initializer through baseclass
  self.radius = radius
end

local c = Circle.new("red", 5)
print(c:getColor())  -- prints "red"
```

### Type Checking
The `is_a` table allows checking instance types and inheritance:

```lua
local Animal = class()
local Dog = class(Animal)
local Car = class()

local spot = Dog.new()

-- Check direct type
print(spot.is_a[Dog])      -- true

-- Check inheritance
print(spot.is_a[Animal])   -- true

-- Check unrelated type
print(spot.is_a[Car])    -- false
```

### Base Class Access
Each class maintains a reference to its base class:

```lua
local Vehicle = class()
local Car = class(Vehicle)

print(Car.baseclass == Vehicle)  -- true
```

## Example: Complete Class Hierarchy

```lua
local class = require 'llae.class'

-- Base class
local Animal = class()

function Animal:_init(name)
  self.name = name
end

function Animal:speak()
  return "..."
end

-- Derived class
local Dog = class(Animal)

function Dog:_init(name, breed)
  Dog.baseclass._init(self, name)  -- Call base class initializer
  self.breed = breed
end

function Dog:speak()
  return "Woof!"
end

-- Usage
local spot = Dog.new("Spot", "Dalmatian")
print(spot.name)           -- "Spot"
print(spot.breed)          -- "Dalmatian"
print(spot:speak())        -- "Woof!"
print(spot.is_a[Dog])      -- true
print(spot.is_a[Animal])   -- true
```

## Implementation Details

1. When a class is created:
   - All methods from the base class are copied to the new class
   - The class's metatable is set up for instance creation
   - The `is_a` table is populated with inheritance information

2. When an instance is created (`new`):
   - A new table is created with the class as its metatable
   - If the class has an `_init` method, it's called with the provided arguments

3. Type checking:
   - The `is_a` table contains both the class references and string names
   - It includes entries for the class itself and all its ancestor classes

## Best Practices

1. Always define initializers using the `_init` method name:
   ```lua
   function MyClass:_init(...)
     -- initialization code
   end
   ```

2. When inheriting, call the base class's initializer through baseclass:
   ```lua
   function Derived:_init(...)
     Derived.baseclass._init(self, ...)
   end
   ```

3. Use the `is_a` table for type checking:
   ```lua
   if obj.is_a[MyClass] then  -- Good
     -- code
   end
   ``` 
# CartForge – E-Commerce Inventory & Order Management System

## Overview

CartForge is a console-based e-commerce backend simulation built using C++ and MySQL.

The system manages products, users, shopping carts, orders, inventory, and sales reports while maintaining inventory consistency through transaction-safe order placement.

The project demonstrates core Software Development Engineering concepts including:

* Object-Oriented Programming (OOP)
* Database Management Systems (DBMS)
* Relational Database Design
* SQL Queries and Joins
* Transactions and ACID Properties
* Inventory Management
* Order Processing
* Reporting and Analytics

---

## Tech Stack

* C++
* MySQL Server 8.0
* MySQL Workbench
* MySQL Client Library
* Git
* GitHub
* VS Code

---

## Features

### Product Management

* Add Product
* View Products
* Search Product
* Update Stock
* Delete Product

### User Management

* Add User
* View Users

### Cart Management

* Add Product to Cart
* View Cart
* Remove Product from Cart

### Order Management

* Place Order
* Stock Validation
* Order Creation
* Order Item Tracking
* Inventory Deduction
* Cart Cleanup
* Order History

### Reports

* Low Stock Products
* Total Sales
* Top Selling Products
* User Order History

---

## Database Schema

### users

Stores customer information.

### products

Stores product details and inventory levels.

### cart_items

Stores products added to a user's cart.

### orders

Stores order-level information.

### order_items

Stores product-level details for each order.

---

## Database Relationships

### One-to-Many

User → Orders

One user can place multiple orders.

### One-to-Many

Order → Order Items

One order can contain multiple products.

### Many-to-Many

Users ↔ Products

Implemented using cart_items and order_items tables.

---

## Transaction-Safe Order Placement

The order placement process uses SQL transactions to maintain inventory consistency.

### Workflow

1. Start Transaction
2. Read User Cart
3. Lock Product Rows
4. Validate Stock Availability
5. Create Order Record
6. Insert Order Items
7. Deduct Product Inventory
8. Clear User Cart
9. Commit Transaction

If any step fails:

* Rollback Transaction
* Restore Original State

---

## Why Transactions?

Transactions prevent partial updates.

Example:

Without transactions:

* Order created
* Stock update fails

Result:

* Invalid database state

With transactions:

* Either all operations succeed
* Or all changes are rolled back

---

## Concurrency Scenario

### Interview Question

What happens if two users attempt to purchase the last item simultaneously?

### Solution

The system uses transaction boundaries and row locking.

The first transaction locks the inventory row.

The second transaction waits until the first transaction completes.

This prevents overselling and ensures inventory consistency.

---

## DBMS Concepts Demonstrated

* Primary Keys
* Foreign Keys
* Constraints
* Indexing
* Normalization
* SQL Joins
* Aggregate Functions
* Transactions
* Commit
* Rollback
* ACID Properties

---

## Project Structure

```text
ecommerce-cpp-mysql/
├── README.md
├── database/
│   ├── schema.sql
│   ├── sample_data.sql
│   └── queries.sql
├── src/
│   ├── main.cpp
│   ├── db_connection.h
│   ├── db_connection.cpp
│   ├── product.h
│   ├── product.cpp
│   ├── user.h
│   ├── user.cpp
│   ├── cart.h
│   ├── cart.cpp
│   ├── order.h
│   └── order.cpp
└── screenshots/
```

## Future Improvements

* REST API Layer
* Authentication
* Admin Dashboard
* Order Cancellation
* Payment Simulation
* Docker Deployment
* Unit Testing

---


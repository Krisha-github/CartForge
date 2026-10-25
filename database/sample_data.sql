-- ============================================================
-- CartForge - sample_data.sql
-- Run after schema.sql:  mysql -u root -p < database/sample_data.sql
-- ============================================================
USE cartforge;

INSERT INTO users (name, email) VALUES
('Krisha', 'krisha@example.com'),
('Rahul',  'rahul@example.com'),
('Aditi',  'aditi@example.com');

INSERT INTO products (product_name, price, stock_quantity) VALUES
('Laptop',         55000.00, 10),
('Wireless Mouse',   799.00, 25),
('Keyboard',        1499.00, 15),
('Monitor',        12000.00,  8),
('USB-C Cable',      299.00, 50),
('Headphones',      2499.00, 12);

-- ============================================================
-- CartForge - schema.sql
-- Creates the database and all tables (MySQL 8.0).
-- Run:  mysql -u root -p < database/schema.sql
-- WARNING: drops and recreates the `cartforge` database.
-- ============================================================

DROP DATABASE IF EXISTS cartforge;
CREATE DATABASE cartforge
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;
USE cartforge;

-- ------------------------------------------------------------
-- users: customers
-- ------------------------------------------------------------
CREATE TABLE users (
    user_id     INT AUTO_INCREMENT PRIMARY KEY,
    name        VARCHAR(100) NOT NULL,
    email       VARCHAR(100) NOT NULL UNIQUE,
    created_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- ------------------------------------------------------------
-- products: catalogue + inventory
-- is_active = 0 means "archived" (soft delete). A product that
-- already appears in past orders is archived instead of being
-- hard-deleted, so order history and sales reports stay intact.
-- ------------------------------------------------------------
CREATE TABLE products (
    product_id      INT AUTO_INCREMENT PRIMARY KEY,
    product_name    VARCHAR(100)  NOT NULL,
    price           DECIMAL(10,2) NOT NULL,
    stock_quantity  INT           NOT NULL,
    is_active       BOOLEAN       NOT NULL DEFAULT TRUE,

    CONSTRAINT chk_products_price CHECK (price >= 0),
    CONSTRAINT chk_products_stock CHECK (stock_quantity >= 0)
) ENGINE=InnoDB;

-- ------------------------------------------------------------
-- cart_items: what each user currently has in their cart.
-- One row per (user, product); quantity is incremented on re-add.
-- Cart rows are disposable, so CASCADE is safe here.
-- ------------------------------------------------------------
CREATE TABLE cart_items (
    cart_id     INT AUTO_INCREMENT PRIMARY KEY,
    user_id     INT NOT NULL,
    product_id  INT NOT NULL,
    quantity    INT NOT NULL,

    CONSTRAINT chk_cart_quantity CHECK (quantity > 0),
    CONSTRAINT uq_cart_user_product UNIQUE (user_id, product_id),

    CONSTRAINT fk_cart_user
        FOREIGN KEY (user_id)    REFERENCES users(user_id)       ON DELETE CASCADE,
    CONSTRAINT fk_cart_product
        FOREIGN KEY (product_id) REFERENCES products(product_id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- ------------------------------------------------------------
-- orders: one row per placed order.
-- RESTRICT: a user who has orders cannot be deleted (keeps history).
-- ------------------------------------------------------------
CREATE TABLE orders (
    order_id      INT AUTO_INCREMENT PRIMARY KEY,
    user_id       INT NOT NULL,
    order_date    TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    total_amount  DECIMAL(12,2) NOT NULL DEFAULT 0,

    CONSTRAINT chk_orders_total CHECK (total_amount >= 0),

    CONSTRAINT fk_orders_user
        FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE RESTRICT
) ENGINE=InnoDB;

-- ------------------------------------------------------------
-- order_items: line items of each order.
-- price_at_purchase is a deliberate snapshot of the price, so later
-- price changes never rewrite history.
-- Deleting an order removes its lines (CASCADE); deleting a product
-- that was ever ordered is blocked (RESTRICT).
-- ------------------------------------------------------------
CREATE TABLE order_items (
    order_item_id      INT AUTO_INCREMENT PRIMARY KEY,
    order_id           INT NOT NULL,
    product_id         INT NOT NULL,
    quantity           INT NOT NULL,
    price_at_purchase  DECIMAL(10,2) NOT NULL,

    CONSTRAINT chk_oi_quantity CHECK (quantity > 0),
    CONSTRAINT chk_oi_price    CHECK (price_at_purchase >= 0),
    CONSTRAINT uq_order_product UNIQUE (order_id, product_id),

    CONSTRAINT fk_oi_order
        FOREIGN KEY (order_id)   REFERENCES orders(order_id)     ON DELETE CASCADE,
    CONSTRAINT fk_oi_product
        FOREIGN KEY (product_id) REFERENCES products(product_id) ON DELETE RESTRICT
) ENGINE=InnoDB;

-- ------------------------------------------------------------
-- Extra indexes (foreign-key columns already get one automatically)
-- ------------------------------------------------------------
CREATE INDEX idx_orders_user_date ON orders(user_id, order_date);
CREATE INDEX idx_products_stock   ON products(stock_quantity);

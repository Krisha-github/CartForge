-- ============================================================
-- CartForge - queries.sql
-- A tour of the SQL behind the application: joins, aggregates,
-- GROUP BY / HAVING, window functions, integrity checks, and the
-- transaction used for checkout.
--
-- Everything in sections 1-6 is READ-ONLY, so you can run the whole
-- file safely:   mysql -u root -p < database/queries.sql
-- (Sections 7-8 are commented out because they change data.)
--
-- The examples use user_id = 1 and product_id = 1; change them freely.
-- Load schema.sql + sample_data.sql and place a few orders first so the
-- reports have something to show.
-- ============================================================
USE cartforge;


-- ============================================================
-- 1. CATALOGUE
-- ============================================================

-- 1.1 All active products
SELECT product_id, product_name, price, stock_quantity
FROM   products
WHERE  is_active = 1
ORDER  BY product_id;

-- 1.2 Search by keyword (what the app's "Search Product" runs)
SELECT product_id, product_name, price, stock_quantity
FROM   products
WHERE  is_active = 1
  AND  product_name LIKE CONCAT('%', 'mouse', '%')
ORDER  BY product_id;

-- 1.3 Products that can be bought right now, cheapest first
SELECT product_name, price
FROM   products
WHERE  is_active = 1 AND stock_quantity > 0
ORDER  BY price ASC;


-- ============================================================
-- 2. CARTS
-- ============================================================

-- 2.1 One user's cart with line subtotals (JOIN)
SELECT c.cart_id,
       p.product_name,
       c.quantity,
       p.price,
       c.quantity * p.price AS subtotal
FROM   cart_items c
JOIN   products   p ON p.product_id = c.product_id
WHERE  c.user_id = 1
ORDER  BY c.cart_id;

-- 2.2 Cart total for a user (aggregate)
SELECT IFNULL(SUM(c.quantity * p.price), 0) AS cart_total
FROM   cart_items c
JOIN   products   p ON p.product_id = c.product_id
WHERE  c.user_id = 1;

-- 2.3 Who currently has items waiting in a cart, and how much is it worth?
SELECT u.user_id,
       u.name,
       COUNT(*)                     AS distinct_items,
       SUM(c.quantity * p.price)    AS cart_value
FROM   cart_items c
JOIN   users      u ON u.user_id    = c.user_id
JOIN   products   p ON p.product_id = c.product_id
GROUP  BY u.user_id, u.name
ORDER  BY cart_value DESC;


-- ============================================================
-- 3. ORDERS
-- ============================================================

-- 3.1 A user's order history, newest first (3-table JOIN)
SELECT o.order_id,
       o.order_date,
       p.product_name,
       oi.quantity,
       oi.price_at_purchase,
       oi.quantity * oi.price_at_purchase AS line_total
FROM   orders      o
JOIN   order_items oi ON oi.order_id   = o.order_id
JOIN   products    p  ON p.product_id  = oi.product_id
WHERE  o.user_id = 1
ORDER  BY o.order_date DESC, o.order_id DESC, oi.order_item_id;

-- 3.2 Orders with how many lines and units each contains
SELECT o.order_id,
       u.name,
       o.order_date,
       COUNT(oi.order_item_id) AS line_items,
       SUM(oi.quantity)        AS units,
       o.total_amount
FROM   orders      o
JOIN   users       u  ON u.user_id  = o.user_id
JOIN   order_items oi ON oi.order_id = o.order_id
GROUP  BY o.order_id, u.name, o.order_date, o.total_amount
ORDER  BY o.order_id;

-- 3.3 Big orders only (HAVING filters AFTER grouping)
SELECT o.order_id, SUM(oi.quantity) AS units, o.total_amount
FROM   orders o
JOIN   order_items oi ON oi.order_id = o.order_id
GROUP  BY o.order_id, o.total_amount
HAVING SUM(oi.quantity) >= 3
ORDER  BY o.total_amount DESC;


-- ============================================================
-- 4. INVENTORY
-- ============================================================

-- 4.1 Low stock (threshold 10)
SELECT product_id, product_name, stock_quantity
FROM   products
WHERE  is_active = 1 AND stock_quantity < 10
ORDER  BY stock_quantity, product_id;

-- 4.2 Out of stock
SELECT product_id, product_name
FROM   products
WHERE  is_active = 1 AND stock_quantity = 0;

-- 4.3 Value of the stock on the shelf
SELECT COUNT(*)                          AS products,
       SUM(stock_quantity)               AS units_in_stock,
       SUM(stock_quantity * price)       AS inventory_value
FROM   products
WHERE  is_active = 1;


-- ============================================================
-- 5. SALES ANALYTICS
-- ============================================================

-- 5.1 Total sales and number of orders
SELECT COUNT(*)                         AS orders_placed,
       IFNULL(SUM(total_amount), 0)     AS total_sales,
       IFNULL(AVG(total_amount), 0)     AS average_order_value
FROM   orders;

-- 5.2 Sales per day
SELECT DATE(order_date)   AS day,
       COUNT(*)           AS orders,
       SUM(total_amount)  AS sales
FROM   orders
GROUP  BY DATE(order_date)
ORDER  BY day DESC;

-- 5.3 Top selling products (units and revenue)
SELECT p.product_name,
       SUM(oi.quantity)                        AS units_sold,
       SUM(oi.quantity * oi.price_at_purchase) AS revenue
FROM   order_items oi
JOIN   products    p ON p.product_id = oi.product_id
GROUP  BY p.product_id, p.product_name
ORDER  BY units_sold DESC, revenue DESC
LIMIT  10;

-- 5.4 Customer ranking by lifetime spend (window function, MySQL 8.0+)
SELECT RANK() OVER (ORDER BY SUM(o.total_amount) DESC) AS spend_rank,
       u.name,
       COUNT(*)              AS orders,
       SUM(o.total_amount)   AS lifetime_spend
FROM   users  u
JOIN   orders o ON o.user_id = u.user_id
GROUP  BY u.user_id, u.name
ORDER  BY spend_rank;

-- 5.5 Users who never ordered (LEFT JOIN + IS NULL anti-join)
SELECT u.user_id, u.name, u.email
FROM   users u
LEFT   JOIN orders o ON o.user_id = u.user_id
WHERE  o.order_id IS NULL;

-- 5.6 Products that have never sold
SELECT p.product_id, p.product_name
FROM   products p
LEFT   JOIN order_items oi ON oi.product_id = p.product_id
WHERE  oi.order_item_id IS NULL AND p.is_active = 1;

-- 5.7 Which products are usually bought together? (self-join on order_items)
SELECT a.product_id AS product_a,
       b.product_id AS product_b,
       COUNT(*)     AS times_together
FROM   order_items a
JOIN   order_items b ON b.order_id = a.order_id AND a.product_id < b.product_id
GROUP  BY a.product_id, b.product_id
ORDER  BY times_together DESC;


-- ============================================================
-- 6. INTEGRITY CHECKS  (each should return ZERO rows)
-- ============================================================

-- 6.1 Order totals that do not match the sum of their lines
SELECT o.order_id, o.total_amount, SUM(oi.quantity * oi.price_at_purchase) AS computed
FROM   orders o
JOIN   order_items oi ON oi.order_id = o.order_id
GROUP  BY o.order_id, o.total_amount
HAVING o.total_amount <> SUM(oi.quantity * oi.price_at_purchase);

-- 6.2 Orders that have no line items
SELECT o.order_id
FROM   orders o
LEFT   JOIN order_items oi ON oi.order_id = o.order_id
WHERE  oi.order_item_id IS NULL;

-- 6.3 Negative stock (also blocked by the CHECK constraint)
SELECT product_id, stock_quantity FROM products WHERE stock_quantity < 0;

-- 6.4 Archived products still sitting in someone's cart
SELECT c.cart_id, c.user_id, c.product_id
FROM   cart_items c
JOIN   products   p ON p.product_id = c.product_id
WHERE  p.is_active = 0;


-- ============================================================
-- 7. CHECKOUT TRANSACTION  (what OrderManager::placeOrder does)
--    Commented out because it changes data. To try it: add an item to
--    user 1's cart, then copy the statements below into MySQL Workbench.
-- ============================================================

START TRANSACTION;

-- 1. Lock the user's cart rows
SELECT product_id, quantity
FROM   cart_items
WHERE  user_id = 1
ORDER  BY product_id
FOR UPDATE;

-- 2. Lock each product row, lowest product_id first (prevents deadlocks),
--    and check stock_quantity >= quantity. If not: ROLLBACK and stop.
SELECT product_name, price, stock_quantity, is_active
FROM   products
WHERE  product_id = 2
FOR UPDATE;

-- 3. Create the order
INSERT INTO orders (user_id) VALUES (1);
SET @order_id = LAST_INSERT_ID();

-- 4. Copy the cart into order_items, snapshotting the current price
INSERT INTO order_items (order_id, product_id, quantity, price_at_purchase)
SELECT @order_id, c.product_id, c.quantity, p.price
FROM   cart_items c
JOIN   products   p ON p.product_id = c.product_id
WHERE  c.user_id = 1;

-- 5. Deduct inventory for every cart line
UPDATE products p
JOIN   cart_items c ON c.product_id = p.product_id AND c.user_id = 1
SET    p.stock_quantity = p.stock_quantity - c.quantity;

-- 6. Compute the order total from the saved lines
UPDATE orders
SET    total_amount = (SELECT SUM(quantity * price_at_purchase)
                       FROM   order_items WHERE order_id = @order_id)
WHERE  order_id = @order_id;

-- 7. Empty the cart
DELETE FROM cart_items WHERE user_id = 1;

COMMIT;      -- or ROLLBACK; to undo everything above


-- ============================================================
-- 8. TWO-SESSION CONCURRENCY DEMO  (last item, two buyers)
--    Open two connections (two query tabs in Workbench).
--    Setup:  UPDATE products SET stock_quantity = 1 WHERE product_id = 1;
-- ============================================================

-- ---- Session A ----                       -- ---- Session B ----
START TRANSACTION;
SELECT stock_quantity FROM products
WHERE product_id = 1 FOR UPDATE;   -- sees 1
                                            START TRANSACTION;
                                            SELECT stock_quantity FROM products
                                            WHERE product_id = 1 FOR UPDATE;
                                            -- ...BLOCKS, waiting for A's lock
UPDATE products
SET stock_quantity = stock_quantity - 1
WHERE product_id = 1;
COMMIT;                            -- lock released
                                            -- B now unblocks and sees 0:
                                            -- stock 0 < quantity 1 => ROLLBACK.
                                            -- The last item was sold exactly once.


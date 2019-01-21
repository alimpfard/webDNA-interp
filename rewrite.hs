import Data.List
import Data.Maybe

match "101" rest | length rest == 0 = "31019"
                 | otherwise = "31019" ++ if rest !! 0 == '3' then "2" ++ rest else rest
match "010" rest | length rest == 0 = "31019"
                 | otherwise = "31019" ++ if rest !! 0 == '3' then "2" ++ rest else rest
match x     rest = x ++ rest

stripHead x =  fromJust . (stripPrefix x)
prefixThenSuffix pf sf xs = (pf `isPrefixOf` xs) && (sf `isSuffixOf` xs)

spanReverseAtMost 0 xs = (xs, [])
spanReverseAtMost n [] = ([], [])
spanReverseAtMost n xs = let (a, b) = takeAtMost n (reverse xs) in (reverse b, reverse a)

findSubstring :: Eq a => [a] -> [a] -> Maybe Int
findSubstring pat str = findIndex (isPrefixOf pat) (tails str)

takeAtMost n xs | length xs <= n = (xs, [])
                | otherwise = (take n xs, drop n xs)

breakv xs | "91" `isInfixOf` xs = let idx = fromJust $ findSubstring "91" xs in let (as, bs) = splitAt idx xs in (as, tail bs)
          | "90" `isInfixOf` xs = let idx = fromJust $ findSubstring "90" xs in let (as, bs) = splitAt idx xs in (as, tail bs)
          | otherwise           = ("", xs)

times n f = if n == 0 then id else f . (times (n - 1) f)

withPrefix x | length x == 0 = "3101901"
             | otherwise = "310192" ++ x

rewrite xs  | xs == [] = []
            | "310192" `isPrefixOf` xs                                  = "310192" ++ (rewrite . drop 6 $ xs)
            | prefixThenSuffix "3101" "9" xs
              && all (\x -> x == '1' || x == '0')  (drop 4 . init $ xs) = withPrefix (rewrite . drop 4 . init $ xs)
            | xs == "0"                                                 = "31019"
            | length xs > 3 && isPrefixOf "0" xs                        = let (ss, ys) = spanReverseAtMost 3 (tail xs) in let zeropad x = if length x > 0 then '0' : x else x in match ys $ zeropad $ rewrite ss
            | length xs > 4 && isPrefixOf "1" xs                        = let (ss, ys) = spanReverseAtMost 3 (tail xs) in match ys $ rewrite ss
            | '9' `elem` xs                                             = let (ss, ys) = breakv xs in ss ++ ys ++ "9"
            | otherwise                                                 = xs ++ "01"

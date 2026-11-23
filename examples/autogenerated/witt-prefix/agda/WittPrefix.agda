{-# OPTIONS --safe --without-K #-}
module WittPrefix where

open import Agda.Builtin.Equality using (_≡_; refl)
open import Agda.Builtin.Nat using (Nat; zero; suc; _+_; _*_)
open import Agda.Builtin.Sigma using (Σ; _,_; fst)

data Prime : Set where
  two three five : Prime

prime-value : Prime → Nat
prime-value two   = 2
prime-value three = 3
prime-value five  = 5

data Digit : Nat → Set where
  first-digit : ∀ {prime} → Digit (suc prime)
  later-digit : ∀ {prime} → Digit prime → Digit (suc prime)

digit-value : ∀ {prime} → Digit prime → Nat
digit-value first-digit = 0
digit-value (later-digit earlier) = suc (digit-value earlier)

data Depth : Set where
  one-coordinate two-coordinates three-coordinates : Depth

_×_ : Set → Set → Set
left × right = Σ left (λ _ → right)

Prefix : Set → Depth → Set
Prefix item one-coordinate = item
Prefix item two-coordinates = item × item
Prefix item three-coordinates = (item × item) × item

data Cut : Depth → Depth → Set where
  keep : ∀ {depth} → Cut depth depth
  from-two-keep-one : Cut two-coordinates one-coordinate
  from-three-keep-two : Cut three-coordinates two-coordinates
  from-three-keep-one : Cut three-coordinates one-coordinate

truncate : ∀ {item source target} → Cut source target →
  Prefix item source → Prefix item target
truncate keep prefix = prefix
truncate from-two-keep-one prefix = fst prefix
truncate from-three-keep-two prefix = fst prefix
truncate from-three-keep-one prefix = fst (fst prefix)

infixr 8 _^_
_^_ : Nat → Nat → Nat
base ^ zero = 1
base ^ suc exponent = base * base ^ exponent

second-ghost : (prime first second : Nat) → Nat
second-ghost prime first second = first ^ prime + prime * second

third-ghost : (prime first second third : Nat) → Nat
third-ghost prime first second third =
  first ^ (prime * prime) + prime * second ^ prime + (prime * prime) * third

evaluate-ghosts : (prime : Prime) (depth : Depth) →
  Prefix (Digit (prime-value prime)) depth → Prefix Nat depth
evaluate-ghosts prime one-coordinate first = digit-value first
evaluate-ghosts prime two-coordinates (first , second) =
  digit-value first , second-ghost (prime-value prime) (digit-value first) (digit-value second)
evaluate-ghosts prime three-coordinates ((first , second) , third) =
  evaluate-ghosts prime two-coordinates (first , second) ,
  third-ghost (prime-value prime) (digit-value first) (digit-value second) (digit-value third)

evaluate-commutes-with-truncation : (prime : Prime) → ∀ {source target}
  (cut : Cut source target) (coordinates : Prefix (Digit (prime-value prime)) source) →
  truncate cut (evaluate-ghosts prime source coordinates) ≡
  evaluate-ghosts prime target (truncate cut coordinates)
evaluate-commutes-with-truncation prime keep coordinates = refl
evaluate-commutes-with-truncation prime from-two-keep-one (first , second) = refl
evaluate-commutes-with-truncation prime from-three-keep-two ((first , second) , third) = refl
evaluate-commutes-with-truncation prime from-three-keep-one ((first , second) , third) = refl

cong : ∀ {a b : Nat} (observe : Nat → Nat) → a ≡ b → observe a ≡ observe b
cong observe refl = refl

right-zero : (value : Nat) → value + 0 ≡ value
right-zero zero = refl
right-zero (suc value) = cong suc (right-zero value)

right-successor : (left right : Nat) → left + suc right ≡ suc (left + right)
right-successor zero right = refl
right-successor (suc left) right = cong suc (right-successor left right)

commute : (left right : Nat) → left + right ≡ right + left
commute zero right rewrite right-zero right = refl
commute (suc left) right rewrite right-successor right left | commute left right = refl

associate : (prefix left right : Nat) → (prefix + left) + right ≡ prefix + (left + right)
associate zero left right = refl
associate (suc prefix) left right = cong suc (associate prefix left right)

exchange-tail-terms : (prefix left right : Nat) → (prefix + left) + right ≡ (prefix + right) + left
exchange-tail-terms prefix left right
  rewrite associate prefix left right | associate prefix right left | commute left right = refl

third-change-balance : (prime : Prime) →
  (first second before after : Digit (prime-value prime)) →
  let p = prime-value prime
      ghost = third-ghost p (digit-value first) (digit-value second)
  in ghost (digit-value before) + (p * p) * digit-value after ≡
     ghost (digit-value after) + (p * p) * digit-value before
third-change-balance prime first second before after =
  let p = prime-value prime in
  exchange-tail-terms
    (digit-value first ^ (p * p) + p * digit-value second ^ p)
    ((p * p) * digit-value before) ((p * p) * digit-value after)

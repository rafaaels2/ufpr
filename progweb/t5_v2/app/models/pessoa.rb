class Pessoa < ApplicationRecord
  has_one :carro, dependent: :destroy
  accepts_nested_attributes_for :carro
end


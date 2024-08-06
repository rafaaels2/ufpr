class Pessoa < ApplicationRecord
  has_one :carro, dependent: :destroy
  has_many :computadors, dependent: :destroy
  has_and_belongs_to_many :disciplinas
  accepts_nested_attributes_for :carro, :computadors, :disciplinas

  validates :rg, :nome, :idade, :cidade, presence: true
  validates_associated :carro
end


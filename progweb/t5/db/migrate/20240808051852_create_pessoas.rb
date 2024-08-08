class CreatePessoas < ActiveRecord::Migration[7.1]
  def change
    create_table :pessoas do |t|
      t.string :rg
      t.string :nome
      t.integer :idade
      t.string :cidade

      t.timestamps
    end
  end
end
